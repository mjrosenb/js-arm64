// -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vim: set ts=8 sts=4 et sw=4 tw=99:
//
// Copyright 2013, ARM Limited
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of ARM Limited nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef jit_arm64_MacroAssembler_arm64_h
#define jit_arm64_MacroAssembler_arm64_h

#include "jit/arm64/Assembler-arm64.h"
#include "jit/arm64/vixl/Debugger-vixl.h"
#include "jit/arm64/vixl/MacroAssembler-vixl.h"
#include "jit/arm64/vixl/VIXL-Globals-vixl.h"

#include "jit/IonFrames.h"
#include "jit/MoveResolver.h"

class Operand {
    // lolwut? it looks like CodeGenerator is accessing this directly?
    // That should probably be changed
};

namespace js {
namespace jit {

struct ImmShiftedTag : public ImmWord
{
    ImmShiftedTag(JSValueShiftedTag shtag)
      : ImmWord((uintptr_t)shtag)
    { }

    ImmShiftedTag(JSValueType type)
      : ImmWord(uintptr_t(JSValueShiftedTag(JSVAL_TYPE_TO_SHIFTED_TAG(type))))
    { }
};

struct ImmTag : public Imm32
{
    ImmTag(JSValueTag tag)
      : Imm32(tag)
    { }
};

class MacroAssemblerCompat : public MacroAssemblerVIXL
{
  protected:
    bool enoughMemory_;
    uint32_t framePushed_;

    // TODO: Can this be moved out of the MacroAssembler and into some shared code?
    // TODO: All the code seems to be arch-independent, and it's weird to have this here.
    bool inCall_;
    bool usedOutParam_;
    uint32_t args_;
    uint32_t passedIntArgs_;
    uint32_t passedFloatArgs_;
    uint32_t passedArgTypes_;
    uint32_t stackForCall_;
    bool dynamicAlignment_;

    MacroAssemblerCompat()
      : MacroAssemblerVIXL(),
        enoughMemory_(true),
        framePushed_(0),
        inCall_(false),
        usedOutParam_(false),
        args_(0),
        passedIntArgs_(0),
        passedFloatArgs_(0),
        passedArgTypes_(0),
        stackForCall_(0),
        dynamicAlignment_(false)
    { }

  protected:
    MoveResolver moveResolver_;

  public:
    bool oom() const {
        // FIXME: jandem is trying to knock out enoughMemory_ now... needs rebasing.
        return Assembler::oom() || !enoughMemory_;
    }
    void doBaseIndex(const CPURegister &rt, const BaseIndex &addr, LoadStoreOp op) {
        // Can use ONLY an indexed-reg load
        if (addr.offset == 0) {
            LoadStoreMacro(rt, MemOperand(ARMRegister(addr.base, 64),
                                          ARMRegister(addr.index, 64),
                                          LSL, unsigned(addr.scale)), op);
            return;
        }
        // TODO: should only add here when we can fit it into a single operand.
        Add(ScratchReg2_64,
            ARMRegister(addr.base, 64),
            Operand(ARMRegister(addr.index, 64),
                    LSL,
                    unsigned(addr.scale)));
        LoadStoreMacro(rt, MemOperand(ScratchReg2_64, addr.offset), op);
    }

    void Push(Register reg) {
        MacroAssemblerVIXL::Push(ARMRegister(reg, 64));
        adjustFrame(sizeof(intptr_t));
    }
    void Push(const Imm32 imm) {
        push(imm);
        adjustFrame(sizeof(intptr_t));
    }
    void Push(const ImmWord imm) {
        push(imm);
        adjustFrame(sizeof(intptr_t));
    }
    void Push(const ImmPtr imm) {
        push(imm);
        adjustFrame(sizeof(intptr_t));
    }
    void Push(const ImmGCPtr ptr) {
        push(ptr);
        adjustFrame(sizeof(intptr_t));
    }
    void Push(FloatRegister f) {
        push(f);
        adjustFrame(sizeof(double));
    }
    void Push(const ValueOperand &val) {
        MacroAssemblerVIXL::Push(ARMRegister(val.valueReg(), 64));
        adjustFrame(sizeof(void *));
    }

    template <typename T>
    void Pop(const T t) {
        pop(t);
        adjustFrame(-1 * (int32_t)(sizeof(T)));
    }

    void push(FloatRegister f) {
        MacroAssemblerVIXL::Push(ARMFPRegister(f, 64));
    }
    void push(Imm32 imm) {
        move32(imm, ScratchReg);
        MacroAssemblerVIXL::Push(ScratchReg64);
    }
    void push(ImmWord imm) {
        Mov(ScratchReg64, imm.value);
        MacroAssemblerVIXL::Push(ScratchReg64);
    }
    void push(ImmPtr imm) {
        movePtr(imm, ScratchReg);
        MacroAssemblerVIXL::Push(ScratchReg64);
    }
    void push(ImmGCPtr imm) {
        movePtr(imm, ScratchReg);
        MacroAssemblerVIXL::Push(ScratchReg64);
    }
    void push(Register reg) {
        MacroAssemblerVIXL::Push(ARMRegister(reg, 64));
    }
    void push(ARMRegister reg) {
        MacroAssemblerVIXL::Push(reg);
    }
    void push(Address a) {
        loadPtr(a, ScratchReg);
        MacroAssemblerVIXL::Push(ScratchReg64);
    }

    void pop(const ValueOperand &v) {
        pop(v.valueReg());
    }
    void pop(const FloatRegister &f) {
        MacroAssemblerVIXL::Pop(ARMRegister(f.code(), 64));
    }
    void pop(Register reg) {
        MacroAssemblerVIXL::Pop(ARMRegister(reg, 64));
    }

    void implicitPop(uint32_t args) {
        MOZ_ASSERT(args % sizeof(intptr_t) == 0);
        adjustFrame(-args);
    }

    // FIXME: This is the same on every arch.
    // FIXME: If we can share framePushed_, we can share this.
    // FIXME: Or just make it at the highest level.
    CodeOffsetLabel PushWithPatch(ImmWord word) {
        framePushed_ += sizeof(word.value);
        return pushWithPatch(word);
    }
    CodeOffsetLabel PushWithPatch(ImmPtr ptr) {
        return PushWithPatch(ImmWord(uintptr_t(ptr.value)));
    }

    uint32_t framePushed() const {
        return framePushed_;
    }
    void adjustFrame(int32_t diff) {
        setFramePushed(framePushed_ + diff);
    }

    void setFramePushed(uint32_t framePushed) {
        framePushed_ = framePushed;
    }

    void reserveStack(uint32_t amount) {
        // TODO: This bumps |sp| every time we reserve using a second register.
        // It would save some instructions if we had a fixed, maximum frame size.
        MacroAssemblerVIXL::Claim(Operand(amount));
        adjustFrame(amount);
    }
    void freeStack(uint32_t amount) {
        MacroAssemblerVIXL::Drop(Operand(amount));
        adjustFrame(-((int32_t)amount));
    }
    void freeStack(Register amount) {
        MacroAssemblerVIXL::Drop(Operand(ARMRegister(amount, 64)));
    }

    // Update sp with the value of the current active stack pointer, if necessary.
    void syncStackPtr() {
        if (!GetStackPointer().Is(sp))
            Add(sp, GetStackPointer(), Operand(0));
    }

    void storeValue(ValueOperand val, const Address &dest) {
        storePtr(val.valueReg(), dest);
    }

    template <typename T>
    void storeValue(JSValueType type, Register reg, const T &dest) {
        tagValue(type, reg, ValueOperand(ScratchReg2));
        storeValue(ValueOperand(ScratchReg2), dest);
    }
    template <typename T>
    void storeValue(const Value &val, const T &dest) {
        moveValue(val, ValueOperand(ScratchReg2));
        storeValue(ValueOperand(ScratchReg2), dest);
    }
    void storeValue(ValueOperand val, BaseIndex dest) {
        storePtr(val.valueReg(), dest);
    }

    template <typename T>
    void storeUnboxedValue(ConstantOrRegister value, MIRType valueType, const T &dest,
                           MIRType slotType)
    {
        MOZ_ASSERT(0 && "storeUnboxedValue");
    }
    void loadValue(Address src, Register val) {
        Ldr(ARMRegister(val, 64), MemOperand(src));
    }
    void loadValue(Address src, ValueOperand val) {
        Ldr(ARMRegister(val.valueReg(), 64), MemOperand(src));
    }
    void loadValue(const BaseIndex &src, ValueOperand val) {
        doBaseIndex(ARMRegister(val.valueReg(), 64), src, LDR_x);
    }
    void tagValue(JSValueType type, Register payload, ValueOperand dest) {
        ARMRegister d(dest.valueReg(), 64);
        ARMRegister p(payload, 64);
        if (type == JSVAL_TYPE_INT32 || type == JSVAL_TYPE_BOOLEAN) {
            // 32-bit values can be tagged with two movk's.
            // or, an ORR instruction, and a movk
            orr(d, p, Operand(JSVAL_SHIFTED_TAG_MAX_DOUBLE));
            movk(d, ImmShiftedTag(type).value & 0xffff00000000);
        } else {
            // load the tag
            if (payload != dest.valueReg()) {
                movePtr(ImmShiftedTag(type), dest.valueReg());
                bfi(d, p, 0, JSVAL_TAG_SHIFT);
            } else {
                bfi(d, p, 0, JSVAL_TAG_SHIFT);
                orPtr(ImmShiftedTag(type), dest.valueReg());
            }
        }
    }
    void pushValue(ValueOperand val) {
        MacroAssemblerVIXL::Push(ARMRegister(val.valueReg(), 64));
    }
    void popValue(ValueOperand val) {
        MacroAssemblerVIXL::Pop(ARMRegister(val.valueReg(), 64));
    }
    void pushValue(const Value &val) {
        moveValue(val, ScratchReg2);
        push(ScratchReg2);
    }
    void pushValue(JSValueType type, Register reg) {
        tagValue(type, reg, ValueOperand(ScratchReg2));
        push(ScratchReg2);
    }
    void pushValue(const Address &addr) {
        loadValue(addr, ScratchReg2);
        push(ScratchReg2);
    }
    void moveValue(const Value &val, Register dest) {
        movePtr(ImmWord(val.asRawBits()), dest);
    }
    void moveValue(const Value &src, const ValueOperand &dest) {
        movePtr(ImmWord(src.asRawBits()), dest.valueReg());
    }
    void moveValue(const ValueOperand &src, const ValueOperand &dest) {
        movePtr(src.valueReg(), dest.valueReg());
    }

    CodeOffsetLabel pushWithPatch(ImmWord imm) {
        CodeOffsetLabel label = movWithPatch(imm, ScratchReg);
        push(ScratchReg);
        return label;
    }

    CodeOffsetLabel movWithPatch(ImmWord imm, Register dest) {
        BufferOffset off = immPool64(ARMRegister(dest, 64), imm.value);
        return CodeOffsetLabel(off.getOffset());
    }
    CodeOffsetLabel movWithPatch(ImmPtr imm, Register dest) {
        BufferOffset off = immPool64(ARMRegister(dest, 64), uint64_t(imm.value));
        return CodeOffsetLabel(off.getOffset());
    }

    void boxValue(JSValueType type, Register src, Register dest) {
        MOZ_ASSERT(0 && "boxValue");
    }
    void splitTag(Register src, Register dest) {
        ubfx(ARMRegister(dest, 64), ARMRegister(src, 64), JSVAL_TAG_SHIFT, (64 - JSVAL_TAG_SHIFT));
    }
    Register extractTag(const Address &address, Register scratch) {
        loadPtr(address, scratch);
        splitTag(scratch, scratch);
        return scratch;
    }
    Register extractTag(const ValueOperand &value, Register scratch) {
        splitTag(value.valueReg(), scratch);
        return scratch;
    }
    Register extractObject(const Address &address, Register scratch) {
        loadPtr(address, scratch);
        unboxObject(scratch, scratch);
        return scratch;
    }
    Register extractObject(const ValueOperand &value, Register scratch) {
        unboxObject(value, scratch);
        return scratch;
    }
    Register extractInt32(const ValueOperand &value, Register scratch) {
        unboxInt32(value, scratch);
        return scratch;
    }
    Register extractBoolean(const ValueOperand &value, Register scratch) {
        unboxBoolean(value, scratch);
        return scratch;
    }

    // If source is a double, load into dest.
    // If source is int32, convert to double and store in dest.
    // Else, branch to failure.
    void ensureDouble(const ValueOperand &source, FloatRegister dest, Label *failure) {
        MOZ_ASSERT(0 && "ensureDouble()");
    }
    void emitSet(Assembler::Condition cond, Register dest) {
        Cset(ARMRegister(dest, 64), cond);
    }

    template <typename T1, typename T2>
    void cmpPtrSet(Assembler::Condition cond, T1 lhs, T2 rhs, Register dest) {
        cmpPtr(lhs, rhs);
        emitSet(cond, dest);
    }

    template <typename T1, typename T2>
    void cmp32Set(Assembler::Condition cond, T1 lhs, T2 rhs, Register dest) {
        cmp32(lhs, rhs);
        emitSet(cond, dest);
    }

    void testNullSet(Condition cond, const ValueOperand &value, Register dest) {
        cond = testNull(cond, value);
        emitSet(cond, dest);
    }
    void testObjectSet(Condition cond, const ValueOperand &value, Register dest) {
        cond = testObject(cond, value);
        emitSet(cond, dest);
    }
    void testUndefinedSet(Condition cond, const ValueOperand &value, Register dest) {
        cond = testUndefined(cond, value);
        emitSet(cond, dest);
    }
    void convertBoolToInt32(Register source, Register dest) {
        Uxtb(ARMRegister(dest, 64), ARMRegister(source, 64));
    }
    void convertInt32ToDouble(Register src, FloatRegister dest) {
        Scvtf(ARMFPRegister(dest, 64), ARMRegister(src, 32)); // Uses FPCR rounding mode.
    }
    void convertInt32ToDouble(const Address &src, FloatRegister dest) {
        MOZ_ASSERT(0 && "convertInt32ToDouble");
    }
    void convertInt32ToFloat32(Register src, FloatRegister dest) {
        Scvtf(ARMFPRegister(dest, 32), ARMRegister(src, 32)); // Uses FPCR rounding mode.
    }
    void convertInt32ToFloat32(const Address &src, FloatRegister dest) {
        MOZ_ASSERT(0 && "convertInt32ToFloat32");
    }

    void convertUInt32ToDouble(const Address &src, FloatRegister dest) {
        MOZ_ASSERT(0 && "convertUInt32ToDouble");
    }
    void convertUInt32ToDouble(Register src, FloatRegister dest) {
        Ucvtf(ARMFPRegister(dest, 64), ARMRegister(src, 32)); // Uses FPCR rounding mode.
    }
    void convertUInt32ToFloat32(Register src, FloatRegister dest) {
        Ucvtf(ARMFPRegister(dest, 32), ARMRegister(src, 32)); // Uses FPCR rounding mode.
    }
    void convertUInt32ToFloat32(const Address &src, FloatRegister dest) {
        MOZ_ASSERT(0 && "convertUInt32ToFloat32");
    }

    void convertFloat32ToDouble(FloatRegister src, FloatRegister dest) {
        MOZ_ASSERT(0 && "convertFloat32ToDouble");
    }
    void convertDoubleToFloat32(FloatRegister src, FloatRegister dest) {
        MOZ_ASSERT(0 && "convertDoubleToFloat32");
    }
    void branchTruncateDouble(FloatRegister src, Register dest, Label *fail) {
        MOZ_ASSERT(0 && "branchTruncateDouble");
    }
    void convertDoubleToInt32(FloatRegister src, Register dest, Label *fail,
                              bool negativeZeroCheck = true)
    {
        MOZ_ASSERT(0 && "convertDoubleToInt32");
    }
    void convertFloat32ToInt32(FloatRegister src, Register dest, Label *fail,
                               bool negativeZeroCheck = true)
    {
        MOZ_ASSERT(0 && "convertFloat32ToInt32");
    }

    void branchTruncateFloat32(FloatRegister src, Register dest, Label *fail) {
        MOZ_ASSERT(0 && "branchTruncateFloat32");
    }
    void jump(Label *label) {
        B(label);
    }
    void jump(RepatchLabel *label) {
        MOZ_ASSERT(0 && "jump (repatchlabel)");
    }
    void jump(Register reg) {
        Br(ARMRegister(reg, 64));
    }
    void jump(const Address &addr) {
        loadPtr(addr, ip0);
        Br(ip0_64);
    }

    void align(int alignment) {
        armbuffer_.align(alignment);
    }

    void movePtr(Register src, Register dest) {
        Mov(ARMRegister(dest, 64), ARMRegister(src, 64));
    }
    void movePtr(ImmWord imm, Register dest) {
        Mov(ARMRegister(dest, 64), (int64_t)imm.value);
    }
    void movePtr(ImmPtr imm, Register dest) {
        Mov(ARMRegister(dest, 64), (int64_t)imm.value);
    }
    void movePtr(AsmJSImmPtr imm, Register dest) {
        MOZ_ASSERT(0 && "movePtr");
    }
    void movePtr(ImmGCPtr imm, Register dest) {
        writeDataRelocation(imm);
        movePatchablePtr(ImmPtr(imm.value), dest);
    }
    void move32(Imm32 imm, Register dest) {
        Mov(ARMRegister(dest, 32), (int64_t)imm.value);
    }
    void move32(Register src, Register dest) {
        Mov(ARMRegister(dest, 32), ARMRegister(src, 32));
    }

    void movePatchablePtr(ImmPtr ptr, Register dest) {
        // FIXME: For the moment, this just moves the pointer normally.
        movePtr(ptr, dest);
    }

    void not32(Register reg) {
        Orn(ARMRegister(reg, 32), wzr, ARMRegister(reg, 32));
    }
    void neg32(Register reg) {
        Neg(ARMRegister(reg, 32), Operand(ARMRegister(reg, 32)));
    }

    void loadPtr(AbsoluteAddress address, Register dest) {
        movePtr(ImmWord((uintptr_t)address.addr), ScratchReg);
        ldr(ARMRegister(dest, 64), MemOperand(ScratchReg64));
    }
    void loadPtr(const Address &address, Register dest) {
        ldr(ARMRegister(dest, 64), MemOperand(address));
    }
    void loadPtr(const BaseIndex &src, Register dest) {
        Register base = src.base;
        uint32_t scale = Imm32::ShiftOf(src.scale).value;

        if (src.offset) {
            Add(ScratchReg64, ARMRegister(base, 64), Operand(int64_t(src.offset)));
            base = ScratchReg;
        }

        ARMRegister dest64(dest, 64);
        ARMRegister base64(base, 64);
        ARMRegister index64(src.index, 64);

        Ldr(dest64, MemOperand(base64, index64, LSL, scale));
    }
    void loadPrivate(const Address &src, Register dest) {
        MOZ_ASSERT(0 && "loadPrivate");
    }

    void store8(Register src, const Address &address) {
        MOZ_ASSERT(0 && "store8");
    }
    void store8(Imm32 imm, const Address &address) {
        MOZ_ASSERT(0 && "store8");
    }
    void store8(Register src, const BaseIndex &address) {
        MOZ_ASSERT(0 && "store8");
    }
    void store8(Imm32 imm, const BaseIndex &address) {
        MOZ_ASSERT(0 && "store8");
    }

    void store16(Register src, const Address &address) {
        MOZ_ASSERT(0 && "store16");
    }
    void store16(Imm32 imm, const Address &address) {
        MOZ_ASSERT(0 && "store16");
    }
    void store16(Register src, const BaseIndex &address) {
        MOZ_ASSERT(0 && "store16");
    }
    void store16(Imm32 imm, const BaseIndex &address) {
        MOZ_ASSERT(0 && "store16");
    }

    void storePtr(ImmWord imm, const Address &address) {
        MOZ_ASSERT(0 && "storePtr");
    }
    void storePtr(ImmPtr imm, const Address &address) {
        Mov(ScratchReg2_64, uint64_t(imm.value));
        Str(ScratchReg2_64, MemOperand(ARMRegister(address.base, 64), address.offset));
    }
    void storePtr(ImmGCPtr imm, const Address &address) {
        MOZ_ASSERT(0 && "storePtr");
    }
    void storePtr(Register src, const Address &address) {
        Str(ARMRegister(src, 64), MemOperand(ARMRegister(address.base, 64), address.offset));
    }
    void storePtr(Register src, const BaseIndex &address) {
        doBaseIndex(ARMRegister(src, 64), address, STR_x);
    }
    void storePtr(Register src, AbsoluteAddress address) {
        Mov(ScratchReg2_64, uint64_t(address.addr));
        Str(ARMRegister(src, 64), MemOperand(ScratchReg2_64));
    }

    void store32(Register src, AbsoluteAddress address) {
        Mov(ScratchReg2_64, uint64_t(address.addr));
        Str(ARMRegister(src, 32), MemOperand(ScratchReg2_64));
    }
    void store32(Imm32 imm, const Address &address) {
        Mov(ScratchReg2_32, uint64_t(imm.value));
        Str(ScratchReg2_32, MemOperand(ARMRegister(address.base, 64), address.offset));
    }
    void store32(Register r, const Address &address) {
        Str(ARMRegister(r, 32), MemOperand(ARMRegister(address.base, 64), address.offset));
    }
    void store32(Imm32 imm, const BaseIndex &address) {
        Mov(ScratchReg2_32, uint64_t(imm.value));
        doBaseIndex(ScratchReg2_32, address, STR_w);
    }
    void store32(Register r, const BaseIndex &address) {
        doBaseIndex(ARMRegister(r, 32), address, STR_w);
    }

    void store32_NoSecondScratch(Imm32 imm, const Address &address) {
        Mov(ScratchReg32, uint64_t(imm.value));
        Str(ScratchReg32, MemOperand(ARMRegister(address.base, 64), address.offset));
    }

    // SIMD.
    void loadAlignedInt32x4(const Address &addr, FloatRegister dest) { MOZ_CRASH("NYI"); }
    void storeAlignedInt32x4(FloatRegister src, const Address &addr) { MOZ_CRASH("NYI"); }
    void loadUnalignedInt32x4(const Address &addr, FloatRegister dest) { MOZ_CRASH("NYI"); }
    void storeUnalignedInt32x4(FloatRegister dest, const Address &addr) { MOZ_CRASH("NYI"); }

    void loadAlignedFloat32x4(const Address &addr, FloatRegister dest) { MOZ_CRASH("NYI"); }
    void storeAlignedFloat32x4(FloatRegister src, const Address &addr) { MOZ_CRASH("NYI"); }
    void loadUnalignedFloat32x4(const Address &addr, FloatRegister dest) { MOZ_CRASH("NYI"); }
    void storeUnalignedFloat32x4(FloatRegister dest, const Address &addr) { MOZ_CRASH("NYI"); }

    void rshiftPtr(Imm32 imm, Register dest) {
        Lsr(ARMRegister(dest, 64), ARMRegister(dest, 64), imm.value);
    }
    void rshiftPtrArithmetic(Imm32 imm, Register dest) {
        Asr(ARMRegister(dest, 64), ARMRegister(dest, 64), imm.value);
    }
    void lshiftPtr(Imm32 imm, Register dest) {
        Lsl(ARMRegister(dest, 64), ARMRegister(dest, 64), imm.value);
    }
    void xorPtr(Imm32 imm, Register dest) {
        Eor(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(imm.value));
    }
    void xor32(Imm32 imm, Register dest) {
        Eor(ARMRegister(dest, 32), ARMRegister(dest, 32), Operand(imm.value));
    }

    void xorPtr(Register src, Register dest) {
        Eor(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(ARMRegister(src, 32)));
    }
    void orPtr(ImmWord imm, Register dest) {
        Orr(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(imm.value));
    }
    void orPtr(Imm32 imm, Register dest) {
        Orr(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(imm.value));
    }
    void orPtr(Register src, Register dest) {
        Orr(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(ARMRegister(src, 64)));
    }
    void or32(Imm32 imm, Register dest) {
        Orr(ARMRegister(dest, 32), ARMRegister(dest, 32), Operand(imm.value));
    }
    void or32(Register src, Register dest) {
        Orr(ARMRegister(dest, 32), ARMRegister(dest, 32), Operand(ARMRegister(src, 32)));
    }
    void or32(Imm32 imm, const Address &dest) {
        load32(dest, ScratchReg2);
        Orr(ScratchReg2_32, ScratchReg2_32, Operand(imm.value));
        store32(ScratchReg2, dest);
    }
    void andPtr(Imm32 imm, Register dest) {
        And(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(imm.value));
    }
    void andPtr(Register src, Register dest) {
        And(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(ARMRegister(src, 64)));
    }
    void and32(Imm32 imm, Register dest) {
        And(ARMRegister(dest, 32), ARMRegister(dest, 32), Operand(imm.value));
    }
    void and32(Register src, Register dest) {
        And(ARMRegister(dest, 32), ARMRegister(dest, 32), Operand(ARMRegister(src, 32)));
    }
    void and32(Imm32 mask, Address dest) {
        load32(dest, ScratchReg2);
        And(ScratchReg2_32, ScratchReg2_32, Operand(mask.value));
        store32(ScratchReg2, dest);
    }
    void and32(Address src, Register dest) {
        load32(src, ScratchReg2);
        And(ARMRegister(dest, 32), ARMRegister(dest, 32), Operand(ScratchReg2_32));
    }

    void testPtr(Register lhs, Register rhs) {
        Tst(ARMRegister(lhs, 64), Operand(ARMRegister(rhs, 64)));
    }
    void test32(Register lhs, Register rhs) {
        Tst(ARMRegister(lhs, 32), Operand(ARMRegister(rhs, 32)));
    }
    void test32(const Address &addr, Imm32 imm) {
        load32(addr, ScratchReg2);
        Tst(ScratchReg2_32, Operand(imm.value));
    }
    void test32(Register lhs, Imm32 rhs) {
        Tst(ARMRegister(lhs, 32), Operand(rhs.value));
    }
    void cmp32(Register lhs, Imm32 rhs) {
        Cmp(ARMRegister(lhs, 32), Operand(rhs.value));
    }
    void cmp32(Register a, Register b) {
        Cmp(ARMRegister(a, 32), Operand(ARMRegister(b, 32)));
    }
    void cmp32(const Operand &lhs, Imm32 rhs) {
        MOZ_ASSERT(0 && "cmp32");
    }
    void cmp32(const Operand &lhs, Register rhs) {
        MOZ_ASSERT(0 && "cmp32");
    }

    void cmpPtr(Register lhs, ImmWord rhs) {
        Cmp(ARMRegister(lhs, 64), Operand(rhs.value));
    }
    void cmpPtr(Register lhs, ImmPtr rhs) {
        Cmp(ARMRegister(lhs, 64), Operand(uint64_t(rhs.value)));
    }
    void cmpPtr(Register lhs, Register rhs) {
        Cmp(ARMRegister(lhs, 64), ARMRegister(rhs, 64));
    }
    void cmpPtr(Register lhs, ImmGCPtr rhs) {
        MOZ_ASSERT(0 && "cmpPtr");
    }

    void cmpPtr(const Address &lhs, Register rhs) {
        Ldr(ScratchReg2_64, MemOperand(ARMRegister(lhs.base, 64), lhs.offset));
        Cmp(ScratchReg2_64, Operand(ARMRegister(rhs, 64)));
    }
    void cmpPtr(const Address &lhs, ImmWord rhs) {
        Ldr(ScratchReg2_64, MemOperand(ARMRegister(lhs.base, 64), lhs.offset));
        Cmp(ScratchReg2_64, Operand(rhs.value));
    }
    void cmpPtr(const Address &lhs, ImmPtr rhs) {
        Ldr(ScratchReg2_64, MemOperand(ARMRegister(lhs.base, 64), lhs.offset));
        Cmp(ScratchReg2_64, Operand(uint64_t(rhs.value)));
    }

    void loadDouble(const Address &src, FloatRegister dest) {
        Ldr(ARMFPRegister(dest, 64), MemOperand(ARMRegister(src.base,64), src.offset));
    }
    void loadDouble(const BaseIndex &src, FloatRegister dest) {
        ARMRegister base(src.base, 64);
        ARMRegister index(src.index, 64);
        if (src.offset == 0) {
            Ldr(ARMFPRegister(dest, 64), MemOperand(base, index, LSL, unsigned(src.scale)));
            return;
        }
        Add(ScratchReg2_64, base, Operand(index, LSL, unsigned(src.scale)));
        Ldr(ARMFPRegister(dest, 64), MemOperand(ScratchReg2_64, src.offset));
    }
    void loadFloatAsDouble(const Address &addr, FloatRegister dest) {
        Ldr(ARMFPRegister(dest, 32), MemOperand(ARMRegister(addr.base,64), addr.offset));
        fcvt(ARMFPRegister(dest, 64), ARMFPRegister(dest, 32));
    }
    void loadFloatAsDouble(const BaseIndex &src, FloatRegister dest) {
        ARMRegister base(src.base, 64);
        ARMRegister index(src.index, 64);
        if (src.offset == 0) {
            Ldr(ARMFPRegister(dest, 32), MemOperand(base, index, LSL, unsigned(src.scale)));
        } else {
            Add(ScratchReg2_64, base, Operand(index, LSL, unsigned(src.scale)));
            Ldr(ARMFPRegister(dest, 32), MemOperand(ScratchReg2_64, src.offset));
        }
        fcvt(ARMFPRegister(dest, 64), ARMFPRegister(dest, 32));
    }

    void loadFloat32(const Address &addr, FloatRegister dest) {
        Ldr(ARMFPRegister(dest, 32), MemOperand(ARMRegister(addr.base,64), addr.offset));
    }
    void loadFloat32(const BaseIndex &src, FloatRegister dest) {
        ARMRegister base(src.base, 64);
        ARMRegister index(src.index, 64);
        if (src.offset == 0) {
            Ldr(ARMFPRegister(dest, 32), MemOperand(base, index, LSL, unsigned(src.scale)));
        } else {
            Add(ScratchReg2_64, base, Operand(index, LSL, unsigned(src.scale)));
            Ldr(ARMFPRegister(dest, 32), MemOperand(ScratchReg2_64, src.offset));
        }
    }

    void storeDouble(FloatRegister src, const Address &dest) {
        Str(ARMFPRegister(src, 64), MemOperand(ARMRegister(dest.base, 64), dest.offset));
    }
    void storeDouble(FloatRegister src, const BaseIndex &dest) {
        doBaseIndex(ARMFPRegister(src, 64), dest, STR_d);
    }
    void storeFloat32(FloatRegister src, Address addr) {
        Str(ARMFPRegister(src, 32), MemOperand(ARMRegister(addr.base, 64), addr.offset));
    }
    void storeFloat32(FloatRegister src, BaseIndex addr) {
        doBaseIndex(ARMFPRegister(src, 32), addr, STR_s);
    }

    void moveDouble(FloatRegister src, FloatRegister dest) {
        fmov(ARMFPRegister(dest, 64), ARMFPRegister(src, 64));
    }
    void zeroDouble(FloatRegister reg) {
        fmov(ARMFPRegister(reg, 64), xzr);
    }
    void zeroFloat32(FloatRegister reg) {
        fmov(ARMFPRegister(reg, 32), wzr);
    }
    void negateDouble(FloatRegister reg) {
        fneg(ARMFPRegister(reg, 64), ARMFPRegister(reg, 64));
    }
    void negateFloat(FloatRegister reg) {
        fneg(ARMFPRegister(reg, 32), ARMFPRegister(reg, 32));
    }
    void addDouble(FloatRegister src, FloatRegister dest) {
        fadd(ARMFPRegister(dest, 64), ARMFPRegister(dest, 64), ARMFPRegister(src, 64));
    }
    void subDouble(FloatRegister src, FloatRegister dest) {
        fsub(ARMFPRegister(dest, 64), ARMFPRegister(dest, 64), ARMFPRegister(src, 64));
    }
    void mulDouble(FloatRegister src, FloatRegister dest) {
        fmul(ARMFPRegister(dest, 64), ARMFPRegister(dest, 64), ARMFPRegister(src, 64));
    }
    void divDouble(FloatRegister src, FloatRegister dest) {
        fdiv(ARMFPRegister(dest, 64), ARMFPRegister(dest, 64), ARMFPRegister(src, 64));
    }

    void moveFloat32(FloatRegister src, FloatRegister dest) {
        fmov(ARMFPRegister(dest, 32), ARMFPRegister(src, 32));
    }
    void moveFloatAsDouble(Register src, FloatRegister dest) {
        MOZ_ASSERT(0 && "moveFloatAsDouble");
    }

    void splitTag(const ValueOperand &operand, Register dest) {
        splitTag(operand.valueReg(), dest);
    }
    void splitTag(const Address &operand, Register dest) {
        loadPtr(operand, dest);
        splitTag(dest, dest);
    }
    void splitTag(const BaseIndex &operand, Register dest) {
        MOZ_ASSERT(0 && "splitTag");
    }

    // Extracts the tag of a value and places it in ScratchReg.
    Register splitTagForTest(const ValueOperand &value) {
        MOZ_ASSERT(0 && "splitTagForTest");
        return Register::FromCode(Registers::x0);
    }
    void cmpTag(const ValueOperand &operand, ImmTag tag) {
        MOZ_ASSERT(0 && "cmpTag");
    }

    void load32(const Address &address, Register dest) {
        Ldr(ARMRegister(dest, 32), MemOperand(ARMRegister(address.base, 64), address.offset));
    }
    void load32(const BaseIndex &src, Register dest) {
        doBaseIndex(ARMRegister(dest, 32), src, LDR_w);
    }
    void load32(AbsoluteAddress address, Register dest) {
        movePtr(ImmWord((uintptr_t)address.addr), ScratchReg);
        ldr(ARMRegister(dest, 32), MemOperand(ARMRegister(ScratchReg, 64)));
    }

    void load8SignExtend(const Address &address, Register dest) {
        Ldrsb(ARMRegister(dest, 32), MemOperand(ARMRegister(address.base, 64), address.offset));
    }
    void load8SignExtend(const BaseIndex &src, Register dest) {
        doBaseIndex(ARMRegister(dest, 32), src, LDRSB_w);
    }

    void load8ZeroExtend(const Address &address, Register dest) {
        Ldrb(ARMRegister(dest, 32), MemOperand(ARMRegister(address.base, 64), address.offset));
    }
    void load8ZeroExtend(const BaseIndex &src, Register dest) {
        doBaseIndex(ARMRegister(dest, 32), src, LDRB_w);
    }

    void load16SignExtend(const Address &address, Register dest) {
        Ldrsh(ARMRegister(dest, 32), MemOperand(ARMRegister(address.base, 64), address.offset));
    }
    void load16SignExtend(const BaseIndex &src, Register dest) {
        doBaseIndex(ARMRegister(dest, 32), src, LDRSH_w);
    }

    void load16ZeroExtend(const Address &address, Register dest) {
        Ldrh(ARMRegister(dest, 32), MemOperand(ARMRegister(address.base, 64), address.offset));
    }
    void load16ZeroExtend(const BaseIndex &src, Register dest) {
        doBaseIndex(ARMRegister(dest, 32), src, LDRH_w);
    }

    void add32(Register src, Register dest) {
        Add(ARMRegister(dest, 32), ARMRegister(src, 32), Operand(ARMRegister(src, 32)));
    }
    void add32(Imm32 imm, Register dest) {
        Add(ARMRegister(dest, 32), ARMRegister(dest, 32), Operand(imm.value));
    }
    void add32(Imm32 imm, const Address &dest) {
        Ldr(ScratchReg2_32, MemOperand(ARMRegister(dest.base, 64), dest.offset));
        Add(ScratchReg2_32, ScratchReg2_32, Operand(imm.value));
        Str(ScratchReg2_32, MemOperand(ARMRegister(dest.base, 64), dest.offset));
    }
    void sub32(Imm32 imm, Register dest) {
        Sub(ARMRegister(dest, 32), ARMRegister(dest, 32), Operand(imm.value));
    }
    void sub32(Register src, Register dest) {
        Sub(ARMRegister(dest, 32), ARMRegister(src, 32), Operand(ARMRegister(src, 32)));
    }

    void addPtr(Register src, Register dest) {
        Add(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(ARMRegister(src, 64)));
    }
    void addPtr(Imm32 imm, Register dest) {
        Add(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(imm.value));
    }
    void addPtr(Imm32 imm, const Address &dest) {
        Ldr(ScratchReg2_64, MemOperand(ARMRegister(dest.base, 64), dest.offset));
        Add(ScratchReg2_64, ScratchReg2_64, Operand(imm.value));
        Str(ScratchReg2_64, MemOperand(ARMRegister(dest.base, 64), dest.offset));
    }
    void addPtr(ImmWord imm, Register dest) {
        Add(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(imm.value));
    }
    void addPtr(ImmPtr imm, Register dest) {
        Add(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(uint64_t(imm.value)));
    }
    void addPtr(const Address &src, Register dest) {
        Ldr(ScratchReg2_64, MemOperand(ARMRegister(src.base, 64), src.offset));
        Add(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(ScratchReg2_64));
    }
    void subPtr(Imm32 imm, Register dest) {
        Sub(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(imm.value));
    }
    void subPtr(Register src, Register dest) {
        Sub(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(ARMRegister(src, 64)));
    }
    void subPtr(const Address &addr, Register dest) {
        Ldr(ScratchReg2_64, MemOperand(ARMRegister(addr.base, 64), addr.offset));
        Sub(ARMRegister(dest, 64), ARMRegister(dest, 64), Operand(ScratchReg2_64));
    }
    void subPtr(Register src, const Address &dest) {
        Ldr(ScratchReg2_64, MemOperand(ARMRegister(dest.base, 64), dest.offset));
        Sub(ScratchReg2_64, ScratchReg2_64, Operand(ARMRegister(src, 64)));
        Str(ScratchReg2_64, MemOperand(ARMRegister(dest.base, 64), dest.offset));
    }
    void mul32(Register src1, Register src2, Register dest, Label *onOver, Label *onZero) {
        Smull(ARMRegister(dest, 64), ARMRegister(src1, 32), ARMRegister(src2, 32));
        if (onOver) {
            Cmp(ARMRegister(dest, 64), Operand(ARMRegister(dest, 32), SXTX));
            B(onOver, Overflow);
        }
        if (onZero) {
            Cbz(ARMRegister(dest, 32), onZero);
        }
    }
    void ret() {
        Ret(); // Branches to lr with a return hint.
    }

    void retn(Imm32 n) {
        // ip0 <- [sp]; sp += n; ret ip0
        Ldr(ip0_64, MemOperand(GetStackPointer(), ptrdiff_t(n.value), PostIndex));
        Ret(ip0_64);
    }

    void j(Condition code , Label *dest) {
        b(dest, code);
    }
    void j(Label *dest) {
        b(dest, Always);
    }

    void branch(Condition cond, Label *label) {
        b(label, cond);
    }
    void branch(JitCode *target) {
        addPendingJump(nextOffset(), ImmPtr(target->raw()), Relocation::JITCODE);
        movePatchablePtr(ImmPtr(target->raw()), ip0);
        Br(ip0_64);
    }

    void branch16(Condition cond, Register lhs, Register rhs, Label *label) {
        MOZ_ASSERT(0 && "branch16");
    }

    void branch32(Condition cond, const Operand &lhs, Register rhs, Label *label) {
        MOZ_ASSERT(0 && "branch32");
    }
    void branch32(Condition cond, const Operand &lhs, Imm32 rhs, Label *label) {
        MOZ_ASSERT(0 && "branch32");
    }
    void branch32(Condition cond, Register lhs, Register rhs, Label *label) {
        cmp32(lhs, rhs);
        b(label, cond);
    }
    void branch32(Condition cond, Register lhs, Imm32 imm, Label *label) {
        cmp32(lhs, imm);
        b(label, cond);
    }
    void branch32(Condition cond, const Address &lhs, Register rhs, Label *label) {
        load32(lhs, ScratchReg);
        branch32(cond, ScratchReg, rhs, label);
    }
    void branch32(Condition cond, const Address &lhs, Imm32 imm, Label *label) {
        load32(lhs, ScratchReg);
        branch32(cond, ScratchReg, imm, label);
    }
    void branch32(Condition cond, AbsoluteAddress lhs, Register rhs, Label *label) {
        movePtr(ImmPtr(lhs.addr), ScratchReg2);
        branch32(cond, Address(ScratchReg2, 0), rhs, label);
    }
    void branch32(Condition cond, AbsoluteAddress lhs, Imm32 rhs, Label *label) {
        movePtr(ImmPtr(lhs.addr), ScratchReg2);
        branch32(cond, Address(ScratchReg2, 0), rhs, label);
    }
    void branch32(Condition cond, BaseIndex lhs, Imm32 rhs, Label *label) {
        MOZ_ASSERT(0 && "branch32 BaseIndex");
    }

    void branchSub32(Condition cond, const Address &lhs, Register rhs, Label *label) {
        MOZ_ASSERT(0 && "branchSub32");
    }
    void branchSub32(Condition cond, const Address &lhs, Imm32 imm, Label *label) {
        MOZ_ASSERT(0 && "branchSub32");
    }
    void branchSub32(Condition cond, Register lhs, Imm32 imm, Label *label) {
        MOZ_ASSERT(0 && "branchSub32");
    }
    void branchSub32(Condition cond, Register lhs, Register rhs, Label *label) {
        MOZ_ASSERT(0 && "branchSub32");
    }
    void branchSub32(Condition cond, AbsoluteAddress lhs, Imm32 rhs, Label *label) {
        MOZ_ASSERT(0 && "branchSub32");
    }
    void branchSub32(Condition cond, AbsoluteAddress lhs, Register rhs, Label *label) {
        MOZ_ASSERT(0 && "branchSub32");
    }

    void branchTest16(Condition cond, Register lhs, Register rhs, Label *label) {
        MOZ_ASSERT(0 && "branchTest16");
    }
    void branchTest32(Condition cond, Register lhs, Register rhs, Label *label) {
        MOZ_ASSERT(cond == Zero || cond == NonZero || cond == Signed || cond == NotSigned);
        // x86 prefers |test foo, foo| to |cmp foo, #0|.
        // Convert the former to the latter for ARM.
        if (lhs == rhs && (cond == Zero || cond == NonZero))
            cmp32(lhs, Imm32(0));
        else
            test32(lhs, rhs);
        B(label, cond);
    }
    void branchTest32(Condition cond, Register lhs, Imm32 imm, Label *label) {
        MOZ_ASSERT(cond == Zero || cond == NonZero || cond == Signed || cond == NotSigned);
        test32(lhs, imm);
        B(label, cond);
    }
    void branchTest32(Condition cond, const Address &address, Imm32 imm, Label *label) {
        load32(address, ScratchReg);
        branchTest32(cond, ScratchReg, imm, label);
    }
    void branchTest32(Condition cond, AbsoluteAddress &address, Imm32 imm, Label *label) {
        MOZ_ASSERT(0 && "branchTest32");
    }
    CodeOffsetJump jumpWithPatch(RepatchLabel *label, Condition cond = Always) {
        MOZ_ASSERT(0 && "jumpWithPatch");
    }
    CodeOffsetJump backedgeJump(RepatchLabel *label) {
        return jumpWithPatch(label);
    }
    template <typename T>
    CodeOffsetJump branchPtrWithPatch(Condition cond, Register reg, T ptr, RepatchLabel *label) {
        MOZ_ASSERT(0 && "branchPtrWithPatch");
    }
    template <typename T>
    CodeOffsetJump branchPtrWithPatch(Condition cond, Address addr, T ptr, RepatchLabel *label) {
        MOZ_ASSERT(0 && "branchPtrWithPatch");
    }

    void branchPtr(Condition cond, AsmJSAbsoluteAddress lhs, Register rhs, Label *label) {
        MOZ_ASSERT(0 && "branchPtr");
    }
    void branchPtr(Condition cond, Address lhs, ImmWord ptr, Label *label) {
        loadPtr(lhs, ScratchReg2);
        branchPtr(cond, ScratchReg2, ptr, label);
    }
    void branchPtr(Condition cond, Address lhs, ImmPtr ptr, Label *label) {
        loadPtr(lhs, ScratchReg2);
        branchPtr(cond, ScratchReg2, ptr, label);
    }
    void branchPtr(Condition cond, Address lhs, Register ptr, Label *label) {
        loadPtr(lhs, ScratchReg2);
        branchPtr(cond, ScratchReg2, ptr, label);
    }
    void branchPtr(Condition cond, Register lhs, ImmWord ptr, Label *label) {
        Mov(ScratchReg64, uint64_t(ptr.value));
        branch(cond, label);
    }
    void branchPtr(Condition cond, Register lhs, ImmPtr rhs, Label *label) {
        Mov(ScratchReg64, uint64_t(rhs.value));
        branch(cond, label);
    }
    void branchPtr(Condition cond, Register lhs, ImmGCPtr ptr, Label *label) {
        MOZ_ASSERT(0 && "branchPtr");
    }
    void branchPtr(Condition cond, Address lhs, ImmGCPtr ptr, Label *label) {
        MOZ_ASSERT(0 && "branchPtr");
    }
    void branchPtr(Condition cond, Register lhs, Register rhs, Label *label) {
        Cmp(ARMRegister(lhs, 64), ARMRegister(rhs, 64));
        B(label, cond);
    }
    void branchPtr(Condition cond, AbsoluteAddress lhs, Register rhs, Label *label) {
        loadPtr(lhs, ScratchReg2);
        branchPtr(cond, ScratchReg2, rhs, label);
    }
    void branchPtr(Condition cond, AbsoluteAddress lhs, ImmWord ptr, Label *label) {
        loadPtr(lhs, ScratchReg2);
        branchPtr(cond, ScratchReg2, ptr, label);
    }

    void branchTestPtr(Condition cond, Register lhs, Register rhs, Label *label) {
        Tst(ARMRegister(lhs, 64), Operand(ARMRegister(rhs, 64)));
        B(label, cond);
    }
    void branchTestPtr(Condition cond, Register lhs, Imm32 imm, Label *label) {
        Tst(ARMRegister(lhs, 64), Operand(imm.value));
        B(label, cond);
    }
    void branchTestPtr(Condition cond, const Address &lhs, Imm32 imm, Label *label) {
        loadPtr(lhs, ScratchReg2);
        branchTestPtr(cond, ScratchReg2, imm, label);
    }
    void branchPrivatePtr(Condition cond, const Address &lhs, ImmPtr ptr, Label *label) {
        branchPtr(cond, lhs, ptr, label);
    }

    void branchPrivatePtr(Condition cond, const Address &lhs, Register ptr, Label *label) {
        branchPtr(cond, lhs, ptr, label);
    }

    void branchPrivatePtr(Condition cond, Register lhs, ImmWord ptr, Label *label) {
        branchPtr(cond, lhs, ptr, label);
    }

    void decBranchPtr(Condition cond, Register lhs, Imm32 imm, Label *label) {
        MOZ_ASSERT(0 && "decBranchPtr");
    }

    void branchTestUndefined(Condition cond, Register tag, Label *label) {
        Condition c = testUndefined(cond, tag);
        B(label, c);
    }
    void branchTestInt32(Condition cond, Register tag, Label *label) {
        Condition c = testInt32(cond, tag);
        B(label, c);
    }
    void branchTestDouble(Condition cond, Register tag, Label *label) {
        Condition c = testDouble(cond, tag);
        B(label, c);
    }
    void branchTestBoolean(Condition cond, Register tag, Label *label) {
        Condition c = testBoolean(cond, tag);
        B(label, c);
    }
    void branchTestNull(Condition cond, Register tag, Label *label) {
        Condition c = testNull(cond, tag);
        B(label, c);
    }
    void branchTestString(Condition cond, Register tag, Label *label) {
        Condition c = testString(cond, tag);
        B(label, c);
    }
    void branchTestSymbol(Condition cond, Register tag, Label *label) {
        Condition c = testSymbol(cond, tag);
        B(label, c);
    }
    void branchTestObject(Condition cond, Register tag, Label *label) {
        Condition c = testObject(cond, tag);
        B(label, c);
    }
    void branchTestNumber(Condition cond, Register tag, Label *label) {
        Condition c = testNumber(cond, tag);
        B(label, c);
    }

    void branchTestUndefined(Condition cond, const Address &address, Label *label) {
        Condition c = testUndefined(cond, address);
        B(label, c);
    }
    void branchTestInt32(Condition cond, const Address &address, Label *label) {
        Condition c = testInt32(cond, address);
        B(label, c);
    }
    void branchTestDouble(Condition cond, const Address &address, Label *label) {
        Condition c = testDouble(cond, address);
        B(label, c);
    }

    // Perform a type-test on a full Value loaded into a register.
    // Clobbers the ScratchReg.
    void branchTestUndefined(Condition cond, const ValueOperand &src, Label *label) {
        Condition c = testUndefined(cond, src);
        B(label, c);
    }
    void branchTestInt32(Condition cond, const ValueOperand &src, Label *label) {
        Condition c = testInt32(cond, src);
        B(label, c);
    }
    void branchTestBoolean(Condition cond, const ValueOperand &src, Label *label) {
        Condition c = testBoolean(cond, src);
        B(label, c);
    }
    void branchTestDouble(Condition cond, const ValueOperand &src, Label *label) {
        Condition c = testDouble(cond, src);
        B(label, c);
    }
    void branchTestNull(Condition cond, const ValueOperand &src, Label *label) {
        Condition c = testNull(cond, src);
        B(label, c);
    }
    void branchTestString(Condition cond, const ValueOperand &src, Label *label) {
        Condition c = testString(cond, src);
        B(label, c);
    }
    void branchTestSymbol(Condition cond, const ValueOperand &src, Label *label) {
        Condition c = testSymbol(cond, src);
        B(label, c);
    }
    void branchTestObject(Condition cond, const ValueOperand &src, Label *label) {
        Condition c = testObject(cond, src);
        B(label, c);
    }
    void branchTestNumber(Condition cond, const ValueOperand &src, Label *label) {
        Condition c = testNumber(cond, src);
        B(label, c);
    }

    // Perform a type-test on a Value addressed by BaseIndex.
    // Clobbers the ScratchReg.
    void branchTestUndefined(Condition cond, const BaseIndex &address, Label *label) {
        Condition c = testUndefined(cond, address);
        B(label, c);
    }
    void branchTestInt32(Condition cond, const BaseIndex &address, Label *label) {
        Condition c = testInt32(cond, address);
        B(label, c);
    }
    void branchTestBoolean(Condition cond, const BaseIndex &address, Label *label) {
        Condition c = testBoolean(cond, address);
        B(label, c);
    }
    void branchTestDouble(Condition cond, const BaseIndex &address, Label *label) {
        Condition c = testDouble(cond, address);
        B(label, c);
    }
    void branchTestNull(Condition cond, const BaseIndex &address, Label *label) {
        Condition c = testNull(cond, address);
        B(label, c);
    }
    void branchTestString(Condition cond, const BaseIndex &address, Label *label) {
        Condition c = testString(cond, address);
        B(label, c);
    }
    void branchTestSymbol(Condition cond, const BaseIndex &address, Label *label) {
        Condition c = testSymbol(cond, address);
        B(label, c);
    }
    void branchTestObject(Condition cond, const BaseIndex &address, Label *label) {
        Condition c = testObject(cond, address);
        B(label, c);
    }
    template <typename T>
    void branchTestGCThing(Condition cond, const T &src, Label *label) {
        Condition c = testGCThing(cond, src);
        B(label, c);
    }
    template <typename T>
    void branchTestPrimitive(Condition cond, const T &t, Label *label) {
        Condition c = testPrimitive(cond, t);
        B(label, c);
    }
    template <typename T>
    void branchTestMagic(Condition cond, const T &t, Label *label) {
        Condition c = testMagic(cond, t);
        B(label, c);
    }
    void branchTestMagicValue(Condition cond, const ValueOperand &val, JSWhyMagic why, Label *label) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        branchTestValue(cond, val, MagicValue(why), label);
    }
    Condition testMagic(Condition cond, const ValueOperand &src) {
        splitTag(src, ScratchReg);
        return testMagic(cond, ScratchReg);
    }

    Condition testError(Condition cond, const ValueOperand &src) {
        return testMagic(cond, src);
    }
    void branchTestValue(Condition cond, const ValueOperand &value, const Value &v, Label *label) {
        MOZ_ASSERT(0 && "branchTestValue");
    }
    void branchTestValue(Condition cond, const Address &valaddr, const ValueOperand &value,
                         Label *label)
    {
        MOZ_ASSERT(0 && "branchTestValue");
    }

    void compareDouble(DoubleCondition cond, FloatRegister lhs, FloatRegister rhs) {
        Fcmp(ARMFPRegister(lhs, 64), ARMFPRegister(rhs, 64));
    }
    void branchDouble(DoubleCondition cond, FloatRegister lhs, FloatRegister rhs, Label *label) {
        MOZ_ASSERT(0 && "branchDouble");
    }

    void compareFloat(DoubleCondition cond, FloatRegister lhs, FloatRegister rhs) {
        MOZ_ASSERT(0 && "compareFloat");
    }
    void branchFloat(DoubleCondition cond, FloatRegister lhs, FloatRegister rhs, Label *label) {
        MOZ_ASSERT(0 && "branchFloat");
    }

    void branchNegativeZero(FloatRegister reg, Register scratch, Label *label) {
        MOZ_ASSERT(0 && "branchNegativeZero");
    }
    void branchNegativeZeroFloat32(FloatRegister reg, Register scratch, Label *label) {
        MOZ_ASSERT(0 && "branchNegativeZeroFloat32");
    }

    void boxDouble(FloatRegister src, const ValueOperand &dest) {
        MOZ_ASSERT(0 && "boxDouble");
    }
    void boxNonDouble(JSValueType type, Register src, const ValueOperand &dest) {
        MOZ_ASSERT(0 && "boxNonDouble");
    }

    // Note that the |dest| register here may be ScratchReg, so we shouldn't use it.
    void unboxInt32(const ValueOperand &src, Register dest) {
        move32(src.valueReg(), dest);
    }
    void unboxInt32(const Address &src, Register dest) {
        load32(src, dest);
    }
    void unboxDouble(const Address &src, FloatRegister dest) {
        MOZ_ASSERT(0 && "unboxDouble");
    }
    void unboxDouble(const ValueOperand &src, FloatRegister dest) {
        MOZ_ASSERT(0 && "unboxDouble");
    }

    void unboxArgObjMagic(const ValueOperand &src, Register dest) {
        MOZ_ASSERT(0 && "unboxArgObjMagic");
    }
    void unboxArgObjMagic(const Address &src, Register dest) {
        MOZ_ASSERT(0 && "unboxArgObjMagic");
    }

    void unboxBoolean(const ValueOperand &src, Register dest) {
        move32(src.valueReg(), dest);
    }
    void unboxBoolean(const Address &src, Register dest) {
        load32(src, dest);
    }

    void unboxMagic(const ValueOperand &src, Register dest) {
        move32(src.valueReg(), dest);
    }
    // Unbox any non-double value into dest. Prefer unboxInt32 or unboxBoolean
    // instead if the source type is known.
    void unboxNonDouble(const ValueOperand &src, Register dest) {
        unboxNonDouble(src.valueReg(), dest);
    }
    void unboxNonDouble(Address src, Register dest) {
        loadPtr(src, dest);
        unboxNonDouble(dest, dest);
    }

    void unboxNonDouble(Register src, Register dest) {
        And(ARMRegister(dest, 64), ARMRegister(src, 64), Operand((1ULL << JSVAL_TAG_SHIFT) - 1ULL));
    }

    void unboxPrivate(const ValueOperand &src, Register dest) {
        ubfx(ARMRegister(dest, 64), ARMRegister(src.valueReg(), 64), 1, JSVAL_TAG_SHIFT - 1);
    }

    void notBoolean(const ValueOperand &val) {
        ARMRegister r(val.valueReg(), 64);
        eor(r, r, Operand(1));
    }
    void unboxObject(const ValueOperand &src, Register dest) {
        unboxNonDouble(src.valueReg(), dest);
    }
    void unboxObject(Register src, Register dest) {
        unboxNonDouble(src, dest);
    }
    void unboxObject(const Address &src, Register dest) {
        loadPtr(src, dest);
        unboxNonDouble(dest, dest);
    }

    void unboxValue(const ValueOperand &src, AnyRegister dest) {
        MOZ_ASSERT(0 && "unboxValue");
    }
    void unboxString(const ValueOperand &operand, Register dest) {
        unboxNonDouble(operand, dest);
    }
    void unboxString(const Address &src, Register dest) {
        unboxNonDouble(src, dest);
    }

    // These two functions use the low 32-bits of the full value register.
    void boolValueToDouble(const ValueOperand &operand, FloatRegister dest) {
        convertInt32ToDouble(operand.valueReg(), dest);
    }
    void int32ValueToDouble(const ValueOperand &operand, FloatRegister dest) {
        convertInt32ToDouble(operand.valueReg(), dest);
    }

    void boolValueToFloat32(const ValueOperand &operand, FloatRegister dest) {
        convertInt32ToFloat32(operand.valueReg(), dest);
    }
    void int32ValueToFloat32(const ValueOperand &operand, FloatRegister dest) {
        convertInt32ToFloat32(operand.valueReg(), dest);
    }

    void loadConstantDouble(double d, FloatRegister dest) {
        MOZ_ASSERT(0 && "loadConstantDouble");
    }
    void loadConstantFloat32(float f, FloatRegister dest) {
        MOZ_ASSERT(0 && "loadConstantFloat32");
    }

    // Register-based tests.
    Condition testUndefined(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, ImmTag(JSVAL_TAG_UNDEFINED));
        return cond;
    }
    Condition testInt32(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, ImmTag(JSVAL_TAG_INT32));
        return cond;
    }
    Condition testBoolean(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, ImmTag(JSVAL_TAG_BOOLEAN));
        return cond;
    }
    Condition testNull(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, ImmTag(JSVAL_TAG_NULL));
        return cond;
    }
    Condition testString(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, ImmTag(JSVAL_TAG_STRING));
        return cond;
    }
    Condition testSymbol(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, ImmTag(JSVAL_TAG_SYMBOL));
        return cond;
    }
    Condition testObject(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, ImmTag(JSVAL_TAG_OBJECT));
        return cond;
    }
    Condition testDouble(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, Imm32(JSVAL_TAG_MAX_DOUBLE));
        return (cond == Equal) ? BelowOrEqual : Above;
    }
    Condition testNumber(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, Imm32(JSVAL_UPPER_INCL_TAG_OF_NUMBER_SET));
        return (cond == Equal) ? BelowOrEqual : Above;
    }
    Condition testGCThing(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, Imm32(JSVAL_LOWER_INCL_TAG_OF_GCTHING_SET));
        return (cond == Equal) ? AboveOrEqual : Below;
    }
    Condition testMagic(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, ImmTag(JSVAL_TAG_MAGIC));
        return cond;
    }
    Condition testPrimitive(Condition cond, Register tag) {
        MOZ_ASSERT(cond == Equal || cond == NotEqual);
        cmp32(tag, Imm32(JSVAL_UPPER_EXCL_TAG_OF_PRIMITIVE_SET));
        return (cond == Equal) ? Below : AboveOrEqual;
    }
    Condition testError(Condition cond, Register tag) {
        return testMagic(cond, tag);
    }

    // ValueOperand-based tests.
    Condition testInt32(Condition cond, const ValueOperand &value) {
        splitTag(value, ScratchReg2);
        return testInt32(cond, ScratchReg2);
    }
    Condition testBoolean(Condition cond, const ValueOperand &value) {
        splitTag(value, ScratchReg2);
        return testBoolean(cond, ScratchReg2);
    }
    Condition testDouble(Condition cond, const ValueOperand &value) {
        splitTag(value, ScratchReg2);
        return testDouble(cond, ScratchReg2);
    }
    Condition testNull(Condition cond, const ValueOperand &value) {
        splitTag(value, ScratchReg2);
        return testNull(cond, ScratchReg2);
    }
    Condition testUndefined(Condition cond, const ValueOperand &value) {
        splitTag(value, ScratchReg2);
        return testUndefined(cond, ScratchReg2);
    }
    Condition testString(Condition cond, const ValueOperand &value) {
        splitTag(value, ScratchReg2);
        return testString(cond, ScratchReg2);
    }
    Condition testSymbol(Condition cond, const ValueOperand &value) {
        splitTag(value, ScratchReg2);
        return testSymbol(cond, ScratchReg2);
    }
    Condition testObject(Condition cond, const ValueOperand &value) {
        splitTag(value, ScratchReg2);
        return testObject(cond, ScratchReg2);
    }
    Condition testNumber(Condition cond, const ValueOperand &value) {
        splitTag(value, ScratchReg2);
        return testNumber(cond, ScratchReg2);
    }
    Condition testPrimitive(Condition cond, const ValueOperand &value) {
        splitTag(value, ScratchReg2);
        return testPrimitive(cond, ScratchReg2);
    }

    // Address-based tests.
    Condition testGCThing(Condition cond, const Address &address) {
        splitTag(address, ScratchReg2);
        return testGCThing(cond, ScratchReg2);
    }
    Condition testMagic(Condition cond, const Address &address) {
        splitTag(address, ScratchReg2);
        return testMagic(cond, ScratchReg2);
    }
    Condition testInt32(Condition cond, const Address &address) {
        splitTag(address, ScratchReg2);
        return testInt32(cond, ScratchReg2);
    }
    Condition testDouble(Condition cond, const Address &address) {
        splitTag(address, ScratchReg2);
        return testDouble(cond, ScratchReg2);
    }
    Condition testBoolean(Condition cond, const Address &address) {
        splitTag(address, ScratchReg2);
        return testBoolean(cond, ScratchReg2);
    }
    Condition testNull(Condition cond, const Address &address) {
        splitTag(address, ScratchReg2);
        return testNull(cond, ScratchReg2);
    }
    Condition testUndefined(Condition cond, const Address &address) {
        splitTag(address, ScratchReg2);
        return testUndefined(cond, ScratchReg2);
    }
    Condition testString(Condition cond, const Address &address) {
        splitTag(address, ScratchReg2);
        return testString(cond, ScratchReg2);
    }
    Condition testSymbol(Condition cond, const Address &address) {
        splitTag(address, ScratchReg2);
        return testSymbol(cond, ScratchReg2);
    }
    Condition testObject(Condition cond, const Address &address) {
        splitTag(address, ScratchReg2);
        return testObject(cond, ScratchReg2);
    }
    Condition testNumber(Condition cond, const Address &address) {
        splitTag(address, ScratchReg2);
        return testNumber(cond, ScratchReg2);
    }

    // BaseIndex-based tests.
    Condition testUndefined(Condition cond, const BaseIndex &src) {
        loadPtr(src, ScratchReg2);
        return testUndefined(cond, ScratchReg2);
    }
    Condition testNull(Condition cond, const BaseIndex &src) {
        loadPtr(src, ScratchReg2);
        return testNull(cond, ScratchReg2);
    }
    Condition testBoolean(Condition cond, const BaseIndex &src) {
        loadPtr(src, ScratchReg2);
        return testBoolean(cond, ScratchReg2);
    }
    Condition testString(Condition cond, const BaseIndex &src) {
        loadPtr(src, ScratchReg2);
        return testString(cond, ScratchReg2);
    }
    Condition testSymbol(Condition cond, const BaseIndex &src) {
        loadPtr(src, ScratchReg2);
        return testSymbol(cond, ScratchReg2);
    }
    Condition testInt32(Condition cond, const BaseIndex &src) {
        loadPtr(src, ScratchReg2);
        return testInt32(cond, ScratchReg2);
    }
    Condition testObject(Condition cond, const BaseIndex &src) {
        loadPtr(src, ScratchReg2);
        return testObject(cond, ScratchReg2);
    }
    Condition testDouble(Condition cond, const BaseIndex &src) {
        loadPtr(src, ScratchReg2);
        return testDouble(cond, ScratchReg2);
    }
    Condition testMagic(Condition cond, const BaseIndex &src) {
        loadPtr(src, ScratchReg2);
        return testMagic(cond, ScratchReg2);
    }
    Condition testGCThing(Condition cond, const BaseIndex &src) {
        loadPtr(src, ScratchReg2);
        return testGCThing(cond, ScratchReg2);
    }


    void branchTestInt32Truthy(bool truthy, const ValueOperand &operand, Label *label) {
        ARMRegister payload(operand.valueReg(), 32);
        Tst(payload, payload);
        B(label, truthy ? NonZero : Zero);
    }

    void branchTestDoubleTruthy(bool truthy, FloatRegister reg, Label *label) {
        Fcmp(ARMFPRegister(reg, 64), 0.0);
        if (!truthy) {
            // falsy values are zero, and NaN.
            branch(Zero, label);
            branch(Overflow, label);
        } else {
            // truthy values are non-zero and not nan.
            // If it is overflow
            Label onFalse;
            branch(Zero, &onFalse);
            branch(Overflow, &onFalse);
            b(label);
            bind(&onFalse);
        }
    }

    void branchTestBooleanTruthy(bool truthy, const ValueOperand &operand, Label *label) {
        ARMRegister payload(operand.valueReg(), 32);
        Tst(payload, payload);
        B(label, truthy ? NonZero : Zero);
    }
    Condition testStringTruthy(bool truthy, const ValueOperand &value) {
        ARMRegister string(value.valueReg(), 64);
        Ldr(ScratchReg64, MemOperand(string, JSString::offsetOfLength()));
        Cmp(ScratchReg64, Operand(0));
        return Condition::Zero;
    }
    void branchTestStringTruthy(bool truthy, const ValueOperand &value, Label *label) {
        ARMRegister string(value.valueReg(), 64);
        Ldr(ScratchReg64, MemOperand(string, JSString::offsetOfLength()));
        Cbnz(ScratchReg64, label);
    }
    template <typename T>
    void loadUnboxedValue(const T &src, MIRType type, AnyRegister dest) {
        MOZ_ASSERT(0 && "loadUnboxedValue");
    }

    void loadInstructionPointerAfterCall(Register dest) {
        MOZ_ASSERT(0 && "loadInstructionPointerAfterCall");
    }

    // Emit a B that can be toggled to a CMP. See ToggleToJmp(), ToggleToCmp().
    CodeOffsetLabel toggledJump(Label *label) {
        BufferOffset offset = b(label, Always);
        CodeOffsetLabel ret(offset.getOffset());
        return ret;
    }

    void writeDataRelocation(ImmGCPtr ptr) {
        if (ptr.value)
            tmpDataRelocations_.append(nextOffset());
    }
    void writePrebarrierOffset(CodeOffsetLabel label) {
        tmpPreBarriers_.append(BufferOffset(label.offset()));
    }

    void computeEffectiveAddress(const Address &address, Register dest) {
        Add(ARMRegister(dest, 64), ARMRegister(address.base, 64), Operand(address.offset));
    }
    void computeEffectiveAddress(const BaseIndex &address, Register dest) {
        ARMRegister dest64(dest, 64);
        ARMRegister base64(address.base, 64);
        ARMRegister index64(address.index, 64);

        Add(dest64, base64, Operand(index64, LSL, address.scale));
        if (address.offset)
            Add(dest64, dest64, Operand(address.offset));
    }

  private:
    void setupABICall(uint32_t args);

  public:
    // Setup a call to C/C++ code, given the number of general arguments it
    // takes. Note that this only supports cdecl.
    //
    // In order for alignment to work correctly, the MacroAssembler must have a
    // consistent view of the stack displacement. It is okay to call "push"
    // manually, however, if the stack alignment were to change, the macro
    // assembler should be notified before starting a call.
    void setupAlignedABICall(uint32_t args) {
        MOZ_ASSERT(0 && "setupAlignedABICall");
    }

    // Sets up an ABI call for when the alignment is not known. This may need a
    // scratch register.
    void setupUnalignedABICall(uint32_t args, Register scratch);

    // Arguments must be assigned to a C/C++ call in order. They are moved
    // in parallel immediately before performing the call. This process may
    // temporarily use more stack, in which case sp-relative addresses will be
    // automatically adjusted. It is extremely important that sp-relative
    // addresses are computed *after* setupABICall(). Furthermore, no
    // operations should be emitted while setting arguments.
    void passABIArg(const MoveOperand &from, MoveOp::Type type);
    void passABIArg(Register reg);
    void passABIArg(FloatRegister reg, MoveOp::Type type);
    void passABIOutParam(Register reg);

  private:
    void callWithABIPre(uint32_t *stackAdjust);
    void callWithABIPost(uint32_t stackAdjust, MoveOp::Type result);

  public:
    // Emits a call to a C/C++ function, resolving all argument moves.
    void callWithABI(void *fun, MoveOp::Type result = MoveOp::GENERAL);
    void callWithABI(Register fun, MoveOp::Type result = MoveOp::GENERAL);
    void callWithABI(AsmJSImmPtr imm, MoveOp::Type result = MoveOp::GENERAL);
    void callWithABI(Address fun, MoveOp::Type result = MoveOp::GENERAL);

    CodeOffsetLabel labelForPatch() {
        MOZ_ASSERT(0 && "labelForPatch");
    }

    void handleFailureWithHandler(void *handler);
    void handleFailureWithHandlerTail();

    // FIXME: This is the same on all platforms. Can be common code?
    void makeFrameDescriptor(Register frameSizeReg, FrameType type) {
        lshiftPtr(Imm32(FRAMESIZE_SHIFT), frameSizeReg);
        orPtr(Imm32(type), frameSizeReg);
    }

    void callWithExitFrame(JitCode *target, Register dynStack) {
        MOZ_ASSERT(0 && "callWithExitFrame");
    }

    // FIXME: See CodeGeneratorX64 calls to noteAsmJSGlobalAccess.
    void patchAsmJSGlobalAccess(CodeOffsetLabel patchAt, uint8_t *code,
                                uint8_t *globalData, unsigned globalDataOffset)
    {
        MOZ_ASSERT(0 && "patchAsmJSGlobalAccess");
    }

    void memIntToValue(Address Source, Address Dest) {
        MOZ_ASSERT(0 && "memIntToValue");
    }

#ifdef JSGC_GENERATIONAL
    void branchPtrInNurseryRange(Condition cond, Register ptr, Register temp, Label *label);
    void branchValueIsNurseryObject(Condition cond, ValueOperand value, Register temp, Label *label);
#endif

    // Builds an exit frame on the stack, with a return address to an internal
    // non-function. Returns offset to be passed to markSafepointAt().
    bool buildFakeExitFrame(Register scratch, uint32_t *offset) {
        MOZ_ASSERT(0 && "buildFakeExitFrame");
        return false;
    }
    void callWithExitFrame(JitCode *target) {
        MOZ_ASSERT(0 && "callWithExitFrame");
    }

    void callIon(Register callee) {
        MOZ_ASSERT(0 && "callIon");
    }

    void appendCallSite(const CallSiteDesc &desc) {
        MOZ_ASSERT(0 && "appendCallSite");
    }

    void call(const CallSiteDesc &desc, Label *label) {
        MOZ_ASSERT(0 && "call");
    }
    void call(const CallSiteDesc &desc, Register reg) {
        MOZ_ASSERT(0 && "call");
    }
    void call(const CallSiteDesc &desc, AsmJSImmPtr imm) {
        MOZ_ASSERT(0 && "call");
    }

    void call(AsmJSImmPtr imm) {
        MOZ_ASSERT(0 && "call(AsmJSImmPtr)");
    }

    void call(Register target) {
        Blr(ARMRegister(target, 64));
    }
    void call(JitCode *target) {
        addPendingJump(nextOffset(), ImmPtr(target->raw()), Relocation::JITCODE);
        movePatchablePtr(ImmPtr(target->raw()), ip0);
        call(ip0); // FIXME: Push something?
    }
    void call(Label *target) {
        Bl(target);
    }
    void callExit(AsmJSImmPtr imm, uint32_t stackArgBytes) {
        MOZ_ASSERT(0 && "callExit");
    }

    void callIonFromAsmJS(Register reg) {
        MOZ_ASSERT(0 && "callIonFromAsmJS");
    }

    // Emit a BLR or NOP instruction. ToggleCall can be used to patch
    // this instruction.
    CodeOffsetLabel toggledCall(JitCode *target, bool enabled) {
        MOZ_CRASH("toggledCall()");
    }

    static size_t ToggledCallSize(uint8_t *code) {
        MOZ_ASSERT(0 && "ToggledCallSize");
    }

    void checkARMRegAlignment(const ARMRegister &reg) {
#ifdef DEBUG
        Label aligned;
        Add(ScratchReg2_64, reg, Operand(0)); // Move even if reg == sp. Avoids xzr.
        Tst(ScratchReg2_64, Operand(StackAlignment - 1));
        B(Zero, &aligned);
        breakpoint();
        bind(&aligned);
        Mov(ScratchReg2_64, xzr); // Clear the scratch register for sanity.
#endif
    }

    void checkStackAlignment() {
#ifdef DEBUG
        checkARMRegAlignment(GetStackPointer());

        // If another register is being used to track pushes, check sp explicitly.
        if (!GetStackPointer().Is(sp))
            checkARMRegAlignment(sp);
#endif
    }

    void abiret() {
        MOZ_ASSERT(0 && "abiret");
    }

    void mulBy3(Register src, Register dest) {
        ARMRegister xdest(dest, 64);
        ARMRegister xsrc(src, 64);
        Add(xdest, xsrc, Operand(xsrc, LSL, 1));
    }

    template <typename T>
    void branchAdd32(Condition cond, T src, Register dest, Label *label) {
        add32(src, dest);
        branch(cond, label);
    }

    template <typename T>
    void branchSub32(Condition cond, T src, Register dest, Label *label) {
        sub32(src, dest);
        branch(cond, label);
    }
    void clampCheck(Register r, Label *handleNotAnInt) {
        MOZ_ASSERT(0 && "clampCheck");
    }

    void memMove32(Address Source, Address Dest) {
        MOZ_ASSERT(0 && "memMove32");
    }
    void memMove64(Address Source, Address Dest) {
        MOZ_ASSERT(0 && "memMove64");
    }

    void stackCheck(ImmWord limitAddr, Label *label) {
        MOZ_ASSERT(0 && "stackCheck");
    }
    void clampIntToUint8(Register reg) {
        MOZ_ASSERT(0 && "clampIntToUint8");
    }

    void incrementInt32Value(const Address &addr) {
        MOZ_ASSERT(0 && "IncrementInt32Value");
    }
    void inc64(AbsoluteAddress dest) {
        MOZ_ASSERT(0 && "inc64");
    }

    void breakpoint();

    void loadAsmJSActivation(Register dest) {
        loadPtr(Address(GlobalReg, AsmJSActivationGlobalDataOffset), dest);
    }
    void loadAsmJSHeapRegisterFromGlobalData() {
        MOZ_ASSERT(0 && "loadAsmJSHeapRegisterFromGlobalData");
    }
    // This moves an un-tagged value from src into a
    // dest that already has the correct tag, and /anything/ in the lower bits
    void monoTagMove(Register dest, Register src) {
        Bfi(ARMRegister(dest, 64), ARMRegister(src, 64), 0, JSVAL_TAG_SHIFT);
    }
    void monoTagMove(ARMRegister dest, ARMRegister src) {
        Bfi(dest, src, 0, JSVAL_TAG_SHIFT);
    }
    // FIXME: Should be in Assembler?
    // FIXME: Should be const?
    uint32_t currentOffset() {
        uint32_t offset = nextOffset().getOffset();
        return offset;
    }

  protected:
    bool buildOOLFakeExitFrame(void *fakeReturnAddr) {
        MOZ_ASSERT(0 && "buildOOLFakeExitFrame");
    }
};

typedef MacroAssemblerCompat MacroAssemblerSpecific;

} // namespace jit
} // namespace js

#endif // jit_arm64_MacroAssembler_arm64_h
