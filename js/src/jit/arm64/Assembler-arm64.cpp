/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "jit/arm64/Assembler-arm64.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"

#include "jscompartment.h"
#include "jsutil.h"

#include "assembler/jit/ExecutableAllocator.h"
#include "gc/Marking.h"
#include "jit/arm64/MacroAssembler-arm64.h"
#include "jit/JitCompartment.h"

using namespace js;
using namespace js::jit;

using mozilla::CountLeadingZeroes32;

// Note this is used for inter-AsmJS calls and may pass arguments and results
// in floating point registers even if the system ABI does not.

ABIArg
ABIArgGenerator::next(MIRType type)
{
    JS_ASSERT(0 && "ABIArgGenerator::next");
    return current_;
}

const Register ABIArgGenerator::NonArgReturnVolatileReg0 = r4;
const Register ABIArgGenerator::NonArgReturnVolatileReg1 = r5;

namespace js {
namespace jit {

// FIXME: Shouldn't this be a static method of Assembler?
void
PatchJump(CodeLocationJump &jump_, CodeLocationLabel label) {
    JS_ASSERT(0 && "PatchJump()");
}

// FIXME: Static, so should be capitalized.
void
Assembler::patchDataWithValueCheck(CodeLocationLabel label, PatchedImmPtr newValue,
                                   PatchedImmPtr expected)
{
    Instruction *i = (Instruction *)label.raw();

    // FIXME: Just emits a breakpoint for now until we can test it.
    AssemblerVIXL::Emit(i, BRK | AssemblerVIXL::ImmException(0x7777));
}

// FIXME: Static, so should be capitalized.
void
Assembler::patchDataWithValueCheck(CodeLocationLabel label, ImmPtr newValue, ImmPtr expected)
{
    patchDataWithValueCheck(label, PatchedImmPtr(newValue.value), PatchedImmPtr(expected.value));
}

} // namespace jit
} // namespace js
