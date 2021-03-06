/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "jit/arm64/Architecture-arm64.h"

#include <cstring>

#include "jit/RegisterSets.h"

namespace js {
namespace jit {

Registers::Code
Registers::FromName(const char *name)
{
    // Check for some register aliases first.
    if (strcmp(name, "ip0") == 0)
        return ip0;
    if (strcmp(name, "ip1") == 0)
        return ip1;
    if (strcmp(name, "fp") == 0)
        return fp;
    if (strcmp(name, "x30") == 0) // Default name "lr"
        return x30;
    if (strcmp(name, "x31") == 0) // Default name "sp"
        return sp;

    for (size_t i = 0; i < Total; i++) {
        if (strcmp(GetName(i), name) == 0)
            return Code(i);
    }

    return invalid_reg;
}

FloatRegisters::Code
FloatRegisters::FromName(const char *name)
{
    for (size_t i = 0; i < Total; i++) {
        if (strcmp(GetName(i), name) == 0)
            return Code(i);
    }

    return invalid_fpreg;
}

FloatRegisterSet
FloatRegister::ReduceSetForPush(const FloatRegisterSet &s)
{
    return s;
}

uint32_t
FloatRegister::GetSizeInBytes(const FloatRegisterSet &s)
{
    return s.size() * sizeof(double);
}

uint32_t
FloatRegister::GetPushSizeInBytes(const FloatRegisterSet &s)
{
    return s.size() * sizeof(double);
}

uint32_t
FloatRegister::getRegisterDumpOffsetInBytes()
{
    // Although registers are 128-bits wide, only the first 64 need saving per ABI.
    return code() * sizeof(double);
}

} // namespace jit
} // namespace js
