/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/MathAlgorithms.h"

#include "jit/arm64/Assembler-arm64.h"
#include "jit/Lowering.h"
#include "jit/MIR.h"

#include "jit/shared/Lowering-shared-inl.h"

using namespace js;
using namespace js::jit;

using mozilla::FloorLog2;

// FIXME: Share with Lowering-x64.
bool
LIRGeneratorARM64::useBox(LInstruction *lir, size_t n, MDefinition *mir,
                          LUse::Policy policy, bool useAtStart)
{
    MOZ_ASSERT(mir->type() == MIRType_Value);

    if (!ensureDefined(mir))
        return false;
    lir->setOperand(n, LUse(mir->virtualRegister(), policy, useAtStart));
    return true;
}

// FIXME: Share with Lowering-x64.
bool
LIRGeneratorARM64::useBoxFixed(LInstruction *lir, size_t n, MDefinition *mir, Register reg1, Register)
{
    MOZ_ASSERT(mir->type() == MIRType_Value);

    if (!ensureDefined(mir))
        return false;
    lir->setOperand(n, LUse(reg1, mir->virtualRegister()));
    return true;
}

// FIXME: Share with Lowering-arm.
LAllocation
LIRGeneratorARM64::useByteOpRegister(MDefinition *mir)
{
    return useRegister(mir);
}

// FIXME: Share with Lowering-arm.
LAllocation
LIRGeneratorARM64::useByteOpRegisterOrNonDoubleConstant(MDefinition *mir)
{
    return useRegisterOrNonDoubleConstant(mir);
}

bool
LIRGeneratorARM64::lowerConstantDouble(double d, MInstruction *mir)
{
    return define(new(alloc()) LDouble(d), mir);
}

bool
LIRGeneratorARM64::lowerConstantFloat32(float d, MInstruction *mir)
{
    return define(new(alloc()) LFloat32(d), mir);
}

bool
LIRGeneratorARM64::visitConstant(MConstant *ins)
{
    if (ins->type() == MIRType_Double)
        return lowerConstantDouble(ins->value().toDouble(), ins);

    if (ins->type() == MIRType_Float32)
        return lowerConstantFloat32(ins->value().toDouble(), ins);

    // Emit non-double constants at their uses.
    if (ins->canEmitAtUses())
        return emitAtUses(ins);

    return LIRGeneratorShared::visitConstant(ins);
}

// FIXME: Share with Lowering-x64.
bool
LIRGeneratorARM64::visitBox(MBox *box)
{
    MDefinition *opd = box->getOperand(0);

    // If the operand is a constant, emit near its uses.
    if (opd->isConstant() && box->canEmitAtUses())
        return emitAtUses(box);

    if (opd->isConstant())
        return define(new(alloc()) LValue(opd->toConstant()->value()), box, LDefinition(LDefinition::BOX));

    LBox *ins = new(alloc()) LBox(useRegister(opd), opd->type());
    return define(ins, box, LDefinition(LDefinition::BOX));
}

// FIXME: Share with Lowering-x64.
bool
LIRGeneratorARM64::visitUnbox(MUnbox *unbox)
{
    MDefinition *box = unbox->getOperand(0);
    LUnboxBase *lir;
    if (IsFloatingPointType(unbox->type()))
        lir = new(alloc()) LUnboxFloatingPoint(useRegisterAtStart(box), unbox->type());
    else
        lir = new(alloc()) LUnbox(useRegisterAtStart(box));

    if (unbox->fallible() && !assignSnapshot(lir, unbox->bailoutKind()))
        return false;

    return define(lir, unbox);
}

// FIXME: Share with Lowering-x64.
bool
LIRGeneratorARM64::visitReturn(MReturn *ret)
{
    MDefinition *opd = ret->getOperand(0);
    MOZ_ASSERT(opd->type() == MIRType_Value);

    LReturn *ins = new(alloc()) LReturn;
    ins->setOperand(0, useFixed(opd, JSReturnReg));
    return add(ins);
}

// x = !y
bool
LIRGeneratorARM64::lowerForALU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir, MDefinition *input)
{
    ins->setOperand(0, ins->snapshot() ? useRegister(input) : useRegisterAtStart(input));
    return define(ins, mir,
                  LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::REGISTER));
}

// z = x+y
bool
LIRGeneratorARM64::lowerForALU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, ins->snapshot() ? useRegister(lhs) : useRegisterAtStart(lhs));
    ins->setOperand(1, ins->snapshot() ? useRegisterOrConstant(rhs) :
                                         useRegisterOrConstantAtStart(rhs));
    return define(ins, mir,
                  LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::REGISTER));
}

bool
LIRGeneratorARM64::lowerForFPU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir, MDefinition *input)
{
    ins->setOperand(0, useRegisterAtStart(input));
    return define(ins, mir,
                  LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::REGISTER));
}

template <size_t Temps>
bool
LIRGeneratorARM64::lowerForFPU(LInstructionHelper<1, 2, Temps> *ins, MDefinition *mir,
                               MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, useRegisterAtStart(lhs));
    ins->setOperand(1, useRegisterAtStart(rhs));
    return define(ins, mir,
                  LDefinition(LDefinition::TypeFrom(mir->type()), LDefinition::REGISTER));
}

template bool LIRGeneratorARM64::lowerForFPU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir,
                                             MDefinition *lhs, MDefinition *rhs);
template bool LIRGeneratorARM64::lowerForFPU(LInstructionHelper<1, 2, 1> *ins, MDefinition *mir,
                                             MDefinition *lhs, MDefinition *rhs);

bool
LIRGeneratorARM64::lowerForBitAndAndBranch(LBitAndAndBranch *baab, MInstruction *mir,
                                         MDefinition *lhs, MDefinition *rhs)
{
    baab->setOperand(0, useRegisterAtStart(lhs));
    baab->setOperand(1, useRegisterOrConstantAtStart(rhs));
    return add(baab, mir);
}

bool
LIRGeneratorARM64::defineUntypedPhi(MPhi *phi, size_t lirIndex)
{
    MOZ_CRASH("defineUntypedPhi");
    return false;
}

void
LIRGeneratorARM64::lowerUntypedPhiInput(MPhi *phi, uint32_t inputPosition, LBlock *block, size_t lirIndex)
{
    MOZ_CRASH("lowerUntypedPhiInput");
}

bool
LIRGeneratorARM64::lowerForShift(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs)
{
    ins->setOperand(0, useRegister(lhs));
    ins->setOperand(1, useRegisterOrConstant(rhs));
    return define(ins, mir);
}

bool
LIRGeneratorARM64::lowerDivI(MDiv *div)
{
    MOZ_CRASH("lowerDivI");
    return false;
}

bool
LIRGeneratorARM64::lowerMulI(MMul *mul, MDefinition *lhs, MDefinition *rhs)
{
    LMulI *lir = new(alloc()) LMulI;
    if (mul->fallible() && !assignSnapshot(lir, Bailout_DoubleOutput))
        return false;
    return lowerForALU(lir, mul, lhs, rhs);
}

bool
LIRGeneratorARM64::lowerModI(MMod *mod)
{
    if (mod->isUnsigned())
        return lowerUMod(mod);

    if (mod->rhs()->isConstant()) {
        int32_t rhs = mod->rhs()->toConstant()->value().toInt32();
        int32_t shift = FloorLog2(rhs);
        if (rhs > 0 && 1 << shift == rhs) {
            LModPowTwoI *lir = new(alloc()) LModPowTwoI(useRegister(mod->lhs()), shift);
            if (mod->fallible() && !assignSnapshot(lir, Bailout_DoubleOutput))
                return false;
            return define(lir, mod);
        }
    }

    LModI *lir = new(alloc()) LModI(useRegister(mod->lhs()), useRegister(mod->rhs()), temp());
    if (mod->fallible() && !assignSnapshot(lir, Bailout_DoubleOutput))
        return false;
    return define(lir, mod);
}

bool
LIRGeneratorARM64::visitPowHalf(MPowHalf *ins)
{
    MDefinition *input = ins->input();
    MOZ_ASSERT(input->type() == MIRType_Double);
    LPowHalfD *lir = new(alloc()) LPowHalfD(useRegisterAtStart(input));
    return defineReuseInput(lir, ins, 0);
}

LTableSwitch *
LIRGeneratorARM64::newLTableSwitch(const LAllocation &in, const LDefinition &inputCopy,
                                       MTableSwitch *tableswitch)
{
    return new(alloc()) LTableSwitch(in, inputCopy, tableswitch);
}

LTableSwitchV *
LIRGeneratorARM64::newLTableSwitchV(MTableSwitch *tableswitch)
{
    return new(alloc()) LTableSwitchV(temp(), tempDouble(), tableswitch);
}

bool
LIRGeneratorARM64::visitGuardShape(MGuardShape *ins)
{
    MOZ_ASSERT(ins->obj()->type() == MIRType_Object);

    LDefinition tempObj = temp(LDefinition::OBJECT);
    LGuardShape *guard = new(alloc()) LGuardShape(useRegister(ins->obj()), tempObj);
    if (!assignSnapshot(guard, ins->bailoutKind()))
        return false;
    if (!add(guard, ins))
        return false;
    return redefine(ins, ins->obj());
}

bool
LIRGeneratorARM64::visitGuardObjectType(MGuardObjectType *ins)
{
    MOZ_ASSERT(ins->obj()->type() == MIRType_Object);

    LDefinition tempObj = temp(LDefinition::OBJECT);
    LGuardObjectType *guard = new(alloc()) LGuardObjectType(useRegister(ins->obj()), tempObj);
    if (!assignSnapshot(guard, Bailout_ObjectIdentityOrTypeGuard))
        return false;
    if (!add(guard, ins))
        return false;
    return redefine(ins, ins->obj());
}

bool
LIRGeneratorARM64::lowerUrshD(MUrsh *mir)
{
    MDefinition *lhs = mir->lhs();
    MDefinition *rhs = mir->rhs();

    MOZ_ASSERT(lhs->type() == MIRType_Int32);
    MOZ_ASSERT(rhs->type() == MIRType_Int32);

    LUrshD *lir = new(alloc()) LUrshD(useRegister(lhs), useRegisterOrConstant(rhs), temp());
    return define(lir, mir);
}

bool
LIRGeneratorARM64::visitAsmJSNeg(MAsmJSNeg *ins)
{
    if (ins->type() == MIRType_Int32)
        return define(new(alloc()) LNegI(useRegisterAtStart(ins->input())), ins);

    if(ins->type() == MIRType_Float32)
        return define(new(alloc()) LNegF(useRegisterAtStart(ins->input())), ins);

    MOZ_ASSERT(ins->type() == MIRType_Double);
    return define(new(alloc()) LNegD(useRegisterAtStart(ins->input())), ins);
}

bool
LIRGeneratorARM64::lowerUDiv(MDiv *div)
{
    MOZ_CRASH("lowerUDiv");
    return false;
}

bool
LIRGeneratorARM64::lowerUMod(MMod *mod)
{
    MOZ_CRASH("lowerUMod");
}

bool
LIRGeneratorARM64::visitAsmJSUnsignedToDouble(MAsmJSUnsignedToDouble *ins)
{
    MOZ_ASSERT(ins->input()->type() == MIRType_Int32);
    LAsmJSUInt32ToDouble *lir = new(alloc()) LAsmJSUInt32ToDouble(useRegisterAtStart(ins->input()));
    return define(lir, ins);
}

bool
LIRGeneratorARM64::visitAsmJSUnsignedToFloat32(MAsmJSUnsignedToFloat32 *ins)
{
    MOZ_ASSERT(ins->input()->type() == MIRType_Int32);
    LAsmJSUInt32ToFloat32 *lir = new(alloc()) LAsmJSUInt32ToFloat32(useRegisterAtStart(ins->input()));
    return define(lir, ins);
}

bool
LIRGeneratorARM64::visitAsmJSLoadHeap(MAsmJSLoadHeap *ins)
{
    MOZ_CRASH("visitAsmJSLoadHeap");
}

bool
LIRGeneratorARM64::visitAsmJSStoreHeap(MAsmJSStoreHeap *ins)
{
    MOZ_CRASH("visitAsmJSStoreHeap");
}

bool
LIRGeneratorARM64::visitAsmJSLoadFuncPtr(MAsmJSLoadFuncPtr *ins)
{
    return define(new(alloc()) LAsmJSLoadFuncPtr(useRegister(ins->index()), temp()), ins);
}

bool
LIRGeneratorARM64::visitAsmJSCompareExchangeHeap(MAsmJSCompareExchangeHeap *ins)
{
    MOZ_CRASH("visitAsmJSCompareExchangeHeap");
}

bool
LIRGeneratorARM64::visitAsmJSAtomicBinopHeap(MAsmJSAtomicBinopHeap *ins)
{
    MOZ_CRASH("visitAsmJSAtomicBinopHeap");
}

bool
LIRGeneratorARM64::lowerTruncateDToInt32(MTruncateToInt32 *ins)
{
    MDefinition *opd = ins->input();
    MOZ_ASSERT(opd->type() == MIRType_Double);

    return define(new(alloc()) LTruncateDToInt32(useRegister(opd), LDefinition::BogusTemp()), ins);
}

bool
LIRGeneratorARM64::lowerTruncateFToInt32(MTruncateToInt32 *ins)
{
    MDefinition *opd = ins->input();
    MOZ_ASSERT(opd->type() == MIRType_Float32);

    return define(new(alloc()) LTruncateFToInt32(useRegister(opd), LDefinition::BogusTemp()), ins);
}

bool
LIRGeneratorARM64::visitStoreTypedArrayElementStatic(MStoreTypedArrayElementStatic *ins)
{
    MOZ_CRASH("NYI");
}

bool
LIRGeneratorARM64::visitForkJoinGetSlice(MForkJoinGetSlice *ins)
{
    MOZ_CRASH("NYI");
}

bool
LIRGeneratorARM64::visitSimdTernaryBitwise(MSimdTernaryBitwise *ins)
{
    MOZ_CRASH("NYI");
}

bool
LIRGeneratorARM64::visitSimdSplatX4(MSimdSplatX4 *ins)
{
    MOZ_CRASH("NYI");
}

bool
LIRGeneratorARM64::visitSimdValueX4(MSimdValueX4 *ins)
{
    MOZ_CRASH("NYI");
}

bool
LIRGeneratorARM64::visitAtomicTypedArrayElementBinop(MAtomicTypedArrayElementBinop *ins)
{
    MOZ_CRASH("NYI");
}

bool
LIRGeneratorARM64::visitCompareExchangeTypedArrayElement(MCompareExchangeTypedArrayElement *ins)
{
    MOZ_CRASH("NYI");
}

bool
LIRGeneratorARM64::visitSubstr(MSubstr *ins)
{
    MOZ_CRASH("NYI");
}
