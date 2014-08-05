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

#ifndef A64_ASSEMBLER_A64_H_
#define A64_ASSEMBLER_A64_H_

#include "jit/arm64/vixl/Assembler-vixl.h"

namespace js {
namespace jit {

class Assembler : public AssemblerVIXL {
  public:
    Assembler()
      : AssemblerVIXL()
    { }
    BufferOffset immPool(ARMRegister dest, uint8_t *value, LoadLiteralOp op);
    BufferOffset immPool64(ARMRegister dest, uint64_t value);
    void bind(Label *label) { bind(label, nextOffset()); }
    void bind(Label *label, BufferOffset boff);
    void bind(RepatchLabel* label);

    void finish() {
        armbuffer_.flushPool();
        AssemblerVIXL::FinalizeCode();

    for (unsigned int i = 0; i < tmpDataRelocations_.length(); i++) {
        int offset = tmpDataRelocations_[i].getOffset();
        int real_offset = offset + armbuffer_.poolSizeBefore(offset);
        dataRelocations_.writeUnsigned(real_offset);
    }

    for (unsigned int i = 0; i < tmpJumpRelocations_.length(); i++) {
        int offset = tmpJumpRelocations_[i].getOffset();
        int real_offset = offset + armbuffer_.poolSizeBefore(offset);
        jumpRelocations_.writeUnsigned(real_offset);
    }

    for (unsigned int i = 0; i < tmpPreBarriers_.length(); i++) {
        int offset = tmpPreBarriers_[i].getOffset();
        int real_offset = offset + armbuffer_.poolSizeBefore(offset);
        preBarriers_.writeUnsigned(real_offset);
    }

    }

    bool oom() const {
        // FIXME: Currently not possible to OOM.
        return false;
    }

    void copyJumpRelocationTable(uint8_t *dest) {
        if (jumpRelocations_.length())
            memcpy(dest, jumpRelocations_.buffer(), jumpRelocations_.length());
    }
    void copyDataRelocationTable(uint8_t *dest) {
        if (dataRelocations_.length())
            memcpy(dest, dataRelocations_.buffer(), dataRelocations_.length());
    }
    void copyPreBarrierTable(uint8_t *dest) {
        if (preBarriers_.length())
            memcpy(dest, preBarriers_.buffer(), preBarriers_.length());
    }

    BufferOffset nextOffset() {
        return armbuffer_.nextOffset();
    }

    bool addCodeLabel(CodeLabel label) {
        JS_ASSERT(0 && "addCodeLabel()");
        return false;
    }
    size_t numCodeLabels() const {
        return codeLabels_.length();
    }
    CodeLabel codeLabel(size_t i) {
        return codeLabels_[i];
    }
    void processCodeLabels(uint8_t *rawCode) {
        for (size_t i = 0; i < codeLabels_.length(); i++) {
            CodeLabel label = codeLabels_[i];
            Bind(rawCode, label.dest(), rawCode + actualOffset(label.src()->offset()));
        }
    }

    void Bind(uint8_t *rawCode, AbsoluteLabel *label, const void *address) {
        uint32_t off = actualOffset(label->offset());
        *reinterpret_cast<const void **>(rawCode + off) = address;
    }

    // Size of the jump relocation table, in bytes.
    size_t jumpRelocationTableBytes() const {
        return jumpRelocations_.length();
    }
    size_t dataRelocationTableBytes() const {
        return dataRelocations_.length();
    }
    size_t preBarrierTableBytes() const {
        return preBarriers_.length();
    }
    unsigned int bytesNeeded() {
        return SizeOfCodeGenerated() +
            jumpRelocationTableBytes() +
            dataRelocationTableBytes() +
            preBarrierTableBytes();
    }

    void trace(JSTracer *trc);

    void writeRelocation(BufferOffset src) {
        tmpJumpRelocations_.append(src);
    }

    // Move our entire pool into the instruction stream. This is to force an
    // opportunistic dump of the pool, preferrably when it is more convenient
    // to do a dump.
    void flushBuffer() {
        JS_ASSERT(0 && "flushBuffer()");
    }
    void enterNoPool(size_t maxInst) {
        JS_ASSERT(0 && "enterNoPool()");
    }
    void leaveNoPool() {
        JS_ASSERT(0 && "leaveNoPool()");
    }

    // The buffer is about to be linked. Ensure any constant pools or
    // excess bookkeeping has been flushed to the instruction stream.
    void flush() {
        // TODO: JS_ASSERT(!isFinished);
        armbuffer_.flushPool();
    }

    int actualOffset(int curOffset) {
        return curOffset;
    }
    int actualIndex(int curOffset) {
        return curOffset;
    }
    int labelOffsetToPatchOffset(int labelOff) {
        return labelOff;
    }
    static uint8_t *PatchableJumpAddress(JitCode *code, uint32_t index) {
        JS_ASSERT(0 && "patchableJumpAddress");
    }
    void executableCopy(uint8_t *buffer){
        // TODO: JS_ASSERT(isFinished);
        armbuffer_.executableCopy(buffer);
        // TODO: AutoFlushICache
    }
    void setPrinter(Sprinter *sp) {
        JS_ASSERT(0 && "setPrinter()");
    }

    static void TraceJumpRelocations(JSTracer *trc, JitCode *code, CompactBufferReader &reader) {
        JS_ASSERT(0 && "TraceJumpRelocations()");
    }
    static void TraceDataRelocations(JSTracer *trc, JitCode *code, CompactBufferReader &reader) {
        JS_ASSERT(0 && "TraceDataRelocations()");
    }

  protected:
    void addPendingJump(BufferOffset src, ImmPtr target, Relocation::Kind kind) {
        enoughMemory_ &= jumps_.append(RelativePatch(target.value, kind));
        if (kind == Relocation::JITCODE)
            writeRelocation(src);
    }

  public:
    static uint32_t PatchWrite_NearCallSize() {
        JS_ASSERT(0 && "PatchWrite_NearCallSize()");
        return 0;
    }

    static uint32_t NopSize() {
        JS_ASSERT(0 && "NopSize()");
        return 4;
    }
    static void PatchWrite_NearCall(CodeLocationLabel start, CodeLocationLabel toCall) {
        JS_ASSERT(0 && "PatchWrite_NearCall()");
    }
    static void PatchDataWithValueCheck(CodeLocationLabel label,
                                        PatchedImmPtr newValue,
                                        PatchedImmPtr expected);

    static void PatchDataWithValueCheck(CodeLocationLabel label,
                                        ImmPtr newValue,
                                        ImmPtr expected);

    static void PatchWrite_Imm32(CodeLocationLabel label, Imm32 imm) {
        JS_ASSERT(0 && "PatchWrite_Imm32()");
    }
    static uint32_t AlignDoubleArg(uint32_t offset) {
        JS_ASSERT(0 && "AlignDoubleArg()");
        return (offset+1)&~1;
    }
    static uint8_t *NextInstruction(uint8_t *instruction, uint32_t *count = nullptr) {
        JS_ASSERT(0 && "NextInstruction()");
        return nullptr;
    }
    static uintptr_t GetPointer(uint8_t *) {
        JS_ASSERT(0 && "GetPointer()");
        return 0;
    }

    // Toggle a jmp or cmp emitted by toggledJump().
    static void ToggleToJmp(CodeLocationLabel inst_) {
        JS_ASSERT(0 && "ToggleToJmp()");
    }
    static void ToggleToCmp(CodeLocationLabel inst_) {
        JS_ASSERT(0 && "ToggleToCmp()");
    }

    static void ToggleCall(CodeLocationLabel inst_, bool enabled) {
        JS_ASSERT(0 && "ToggleCall()");
    }

    static void updateBoundsCheck(uint32_t logHeapSize, Instruction *inst) {
        JS_ASSERT(0 && "updateBoundsCheck()");
    }

    static int32_t ExtractCodeLabelOffset(uint8_t *code) {
        JS_ASSERT(0 && "ExtractCodeLabelOffset");
        return 0;
    }
    static void PatchInstructionImmediate(uint8_t *code, PatchedImmPtr imm) {
        JS_ASSERT(0 && "PatchInstructionImmediate");
    }

  protected:
    // Structure for fixing up pc-relative loads/jumps when the machine
    // code gets moved (executable copy, gc, etc.).
    struct RelativePatch
    {
        void *target;
        Relocation::Kind kind;

        RelativePatch(void *target, Relocation::Kind kind)
          : target(target), kind(kind)
        { }
    };

    js::Vector<CodeLabel, 0, SystemAllocPolicy> codeLabels_;
    js::Vector<RelativePatch, 8, SystemAllocPolicy> jumps_;
    js::Vector<BufferOffset, 0, SystemAllocPolicy> tmpJumpRelocations_;
    js::Vector<BufferOffset, 0, SystemAllocPolicy> tmpDataRelocations_;
    js::Vector<BufferOffset, 0, SystemAllocPolicy> tmpPreBarriers_;

    CompactBufferWriter jumpRelocations_;
    CompactBufferWriter dataRelocations_;
    CompactBufferWriter relocations_;
    CompactBufferWriter preBarriers_;
};

class ABIArgGenerator
{
  public:
    ABIArgGenerator()
      : intRegIndex_(0),
        floatRegIndex_(0),
        stackOffset_(0),
        current_()
    { }

    ABIArg next(MIRType argType);
    ABIArg &current() { return current_; }
    uint32_t stackBytesConsumedSoFar() const { return stackOffset_; }

  public:
    static const Register NonArgReturnReg0;
    static const Register NonArgReturnReg1;
    static const Register NonVolatileReg;
    static const Register NonArg_VolatileReg;
    static const Register NonReturn_VolatileReg0;
    static const Register NonReturn_VolatileReg1;

  protected:
    unsigned intRegIndex_;
    unsigned floatRegIndex_;
    uint32_t stackOffset_;
    ABIArg current_;
};

// FIXME: ugh. why is this not a static member of Assembler?
void PatchJump(CodeLocationJump &jump_, CodeLocationLabel label);

static const uint32_t NumIntArgRegs = 8;
static const uint32_t NumFloatArgRegs = 8;

static inline bool
GetIntArgReg(uint32_t usedIntArgs, uint32_t usedFloatArgs, Register *out)
{
    if (usedIntArgs >= NumIntArgRegs)
        return false;
    *out = Register::FromCode(usedIntArgs);
    return true;
}

static inline bool
GetFloatArgReg(uint32_t usedIntArgs, uint32_t usedFloatArgs, FloatRegister *out)
{
    if (usedFloatArgs >= NumFloatArgRegs)
        return false;
    *out = FloatRegister::FromCode(usedFloatArgs);
    return true;
}

// Get a register in which we plan to put a quantity that will be used as an
// integer argument.  This differs from GetIntArgReg in that if we have no more
// actual argument registers to use we will fall back on using whatever
// CallTempReg* don't overlap the argument registers, and only fail once those
// run out too.
static inline bool
GetTempRegForIntArg(uint32_t usedIntArgs, uint32_t usedFloatArgs, Register *out)
{
    JS_ASSERT(0 && "TODO");
    return false;
}

// FIXME: Should be shared with ARM's Assembler.
class AutoForbidPools {
    Assembler *asm_;

  public:
    AutoForbidPools(Assembler *asm_, size_t maxInst)
      : asm_(asm_)
    {
        asm_->enterNoPool(maxInst);
    }

    ~AutoForbidPools() {
        asm_->leaveNoPool();
    }
};

static const uint32_t AlignmentAtPrologue = 0;
static const uint32_t AlignmentMidPrologue = 8;
static const Scale ScalePointer = TimesEight;
static const uint32_t AlignmentAtAsmJSPrologue = sizeof(void*);

static MOZ_CONSTEXPR_VAR Register ScratchReg = { Registers::ip0 };
static MOZ_CONSTEXPR_VAR ARMRegister ScratchReg64 = { ScratchReg, 64 };
static MOZ_CONSTEXPR_VAR ARMRegister ScratchReg32 = { ScratchReg, 32 };

static MOZ_CONSTEXPR_VAR Register ScratchReg2 = { Registers::ip1 };
static MOZ_CONSTEXPR_VAR ARMRegister ScratchReg2_64 = { ScratchReg2, 64 };
static MOZ_CONSTEXPR_VAR ARMRegister ScratchReg2_32 = { ScratchReg2, 32 };

static MOZ_CONSTEXPR_VAR FloatRegister ScratchDoubleReg = { FloatRegisters::d31 };
static MOZ_CONSTEXPR_VAR FloatRegister ReturnDoubleReg = { FloatRegisters::d0 };

static MOZ_CONSTEXPR_VAR FloatRegister ReturnFloat32Reg = { FloatRegisters::s0 };
static MOZ_CONSTEXPR_VAR FloatRegister ScratchFloat32Reg = { FloatRegisters::s31 };

static MOZ_CONSTEXPR_VAR Register OsrFrameReg = { Registers::x5 };
static MOZ_CONSTEXPR_VAR Register ArgumentsRectifierReg = { Registers::x8 };
static MOZ_CONSTEXPR_VAR Register CallTempReg0 = { Registers::x5 };
static MOZ_CONSTEXPR_VAR Register CallTempReg1 = { Registers::x6 };
static MOZ_CONSTEXPR_VAR Register CallTempReg2 = { Registers::x7 };
static MOZ_CONSTEXPR_VAR Register CallTempReg3 = { Registers::x8 };
static MOZ_CONSTEXPR_VAR Register CallTempReg4 = { Registers::x0 };
static MOZ_CONSTEXPR_VAR Register CallTempReg5 = { Registers::x1 };

static MOZ_CONSTEXPR_VAR Register PreBarrierReg = { Registers::x1 };

static MOZ_CONSTEXPR_VAR Register InvalidReg = { Registers::invalid_reg };
static MOZ_CONSTEXPR_VAR FloatRegister InvalidFloatReg = { FloatRegisters::invalid_fpreg };

static MOZ_CONSTEXPR_VAR Register ReturnReg_ = { Registers::x0 };
static MOZ_CONSTEXPR_VAR Register ReturnReg = { Registers::x0 };
static MOZ_CONSTEXPR_VAR Register JSReturnReg = { Registers::x2 };
static MOZ_CONSTEXPR_VAR Register FramePointer = { Registers::fp };
static MOZ_CONSTEXPR_VAR Register StackPointer = { Registers::sp };
static MOZ_CONSTEXPR_VAR Register ZeroRegister = { Registers::sp };
static MOZ_CONSTEXPR_VAR Register PseudoStackPointer = { Registers::x28 };
static MOZ_CONSTEXPR_VAR FloatRegister ReturnFloatReg = { FloatRegisters::d0 };
static MOZ_CONSTEXPR_VAR FloatRegister ScratchFloatReg = { FloatRegisters::d31 };

static MOZ_CONSTEXPR_VAR Register IntArgReg0 = { Registers::x0 };
static MOZ_CONSTEXPR_VAR Register IntArgReg1 = { Registers::x1 };
static MOZ_CONSTEXPR_VAR Register IntArgReg2 = { Registers::x2 };
static MOZ_CONSTEXPR_VAR Register IntArgReg3 = { Registers::x3 };
static MOZ_CONSTEXPR_VAR Register IntArgReg4 = { Registers::x4 };
static MOZ_CONSTEXPR_VAR Register IntArgReg5 = { Registers::x5 };
static MOZ_CONSTEXPR_VAR Register IntArgReg6 = { Registers::x6 };
static MOZ_CONSTEXPR_VAR Register IntArgReg7 = { Registers::x7 };
static MOZ_CONSTEXPR_VAR Register GlobalReg =  { Registers::x10 };
static MOZ_CONSTEXPR_VAR Register HeapReg = { Registers::x11 };

static MOZ_CONSTEXPR_VAR Register r0 = { Registers::x0 };
static MOZ_CONSTEXPR_VAR Register r1 = { Registers::x1 };
static MOZ_CONSTEXPR_VAR Register r2 = { Registers::x2 };
static MOZ_CONSTEXPR_VAR Register r3 = { Registers::x3 };
static MOZ_CONSTEXPR_VAR Register r4 = { Registers::x4 };
static MOZ_CONSTEXPR_VAR Register r5 = { Registers::x5 };
static MOZ_CONSTEXPR_VAR Register r6 = { Registers::x6 };
static MOZ_CONSTEXPR_VAR Register r7 = { Registers::x7 };
static MOZ_CONSTEXPR_VAR Register r8 = { Registers::x8 };
static MOZ_CONSTEXPR_VAR Register r9 = { Registers::x9 };
static MOZ_CONSTEXPR_VAR Register r10 = { Registers::x10 };
static MOZ_CONSTEXPR_VAR Register r11 = { Registers::x11 };
static MOZ_CONSTEXPR_VAR Register r12 = { Registers::x12 };
static MOZ_CONSTEXPR_VAR Register r13 = { Registers::x13 };
static MOZ_CONSTEXPR_VAR Register r14 = { Registers::x14 };
static MOZ_CONSTEXPR_VAR Register r15 = { Registers::x15 };
static MOZ_CONSTEXPR_VAR Register r16 = { Registers::x16 };
static MOZ_CONSTEXPR_VAR Register r17 = { Registers::x17 };
static MOZ_CONSTEXPR_VAR Register r18 = { Registers::x18 };
static MOZ_CONSTEXPR_VAR Register r19 = { Registers::x19 };
static MOZ_CONSTEXPR_VAR Register r20 = { Registers::x20 };
static MOZ_CONSTEXPR_VAR Register r21 = { Registers::x21 };
static MOZ_CONSTEXPR_VAR Register r22 = { Registers::x22 };
static MOZ_CONSTEXPR_VAR Register r23 = { Registers::x23 };
static MOZ_CONSTEXPR_VAR Register r24 = { Registers::x24 };
static MOZ_CONSTEXPR_VAR Register r25 = { Registers::x25 };
static MOZ_CONSTEXPR_VAR Register r26 = { Registers::x26 };
static MOZ_CONSTEXPR_VAR Register r27 = { Registers::x27 };
static MOZ_CONSTEXPR_VAR Register r28 = { Registers::x28 };
static MOZ_CONSTEXPR_VAR Register r29 = { Registers::x29 };
static MOZ_CONSTEXPR_VAR Register r30 = { Registers::x30 };
static MOZ_CONSTEXPR_VAR ValueOperand JSReturnOperand = ValueOperand(JSReturnReg);

// Registers used in the GenerateFFIIonExit Enable Activation block.
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegCallee = r8;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE0 = r0;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE1 = r1;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE2 = r2;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegE3 = r3;

// Registers used in the GenerateFFIIonExit Disable Activation block.
// None of these may be the second scratch register.
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegReturnData = r2;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegReturnType = r3;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegD0 = r0;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegD1 = r1;
static MOZ_CONSTEXPR_VAR Register AsmJSIonExitRegD2 = r4;

static MOZ_CONSTEXPR_VAR Register JSReturnReg_Type = r3;
static MOZ_CONSTEXPR_VAR Register JSReturnReg_Data = r2;

static MOZ_CONSTEXPR_VAR FloatRegister NANReg = { FloatRegisters::d14 };

} // namespace jit
} // namespace js

#endif // A64_ASSEMBLER_A64_H_
