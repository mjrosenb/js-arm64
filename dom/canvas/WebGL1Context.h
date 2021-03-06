/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WEBGL_1_CONTEXT_H_
#define WEBGL_1_CONTEXT_H_

#include "WebGLContext.h"

namespace mozilla {

class WebGL1Context
    : public WebGLContext
{
public:
    static WebGL1Context* Create();

private:
    WebGL1Context();

public:
    virtual ~WebGL1Context();

    virtual bool IsWebGL2() const MOZ_OVERRIDE {
        return false;
    }

    // nsWrapperCache
    virtual JSObject* WrapObject(JSContext* cx) MOZ_OVERRIDE;

private:
    virtual bool ValidateAttribPointerType(bool integerMode, GLenum type, GLsizei* alignment, const char* info) MOZ_OVERRIDE;
    virtual bool ValidateBufferTarget(GLenum target, const char* info) MOZ_OVERRIDE;
    virtual bool ValidateBufferIndexedTarget(GLenum target, const char* info) MOZ_OVERRIDE;
};

} // namespace mozilla

#endif // WEBGL_1_CONTEXT_H_
