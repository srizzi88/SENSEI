/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLState.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLState.h"
#include "svtk_glew.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLFramebufferObject.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLShaderCache.h"
#include "svtkOpenGLVertexBufferObjectCache.h"
#include "svtkRenderer.h"
#include "svtkTextureUnitManager.h"

// must be included after a svtkObject subclass
#include "svtkOpenGLError.h"

#include <cmath>

#include "svtksys/SystemInformation.hxx"

// If you define NO_CACHE then all state->svtkgl* calls
// will get passed down to OpenGL regardless of the current
// state. This basically bypasses the caching mechanism
// and is useful for tesing
// #define NO_CACHE 1

//////////////////////////////////////////////////////////////////////////////
//
// if SVTK_REPORT_OPENGL_ERRORS is defined (done in CMake and svtkOpenGLError.h)
// then a few things change here.
//
// 1) the error code of OpenGL will be checked after every call.
// 2) On an error the full stack trace available will be
//    printed.
// 3) All methods check the cache state to see if anything is out of sync
//
// #undef SVTK_REPORT_OPENGL_ERRORS
#ifdef SVTK_REPORT_OPENGL_ERRORS

// this method checks all the cached state to make sure
// nothing is out of sync. It can be slow.
void svtkOpenGLState::CheckState()
{
  bool error = false;

  GLboolean params[4];

  ::glGetBooleanv(GL_DEPTH_WRITEMASK, params);
  if (params[0] != this->CurrentState.DepthMask)
  {
    svtkGenericWarningMacro("Error in cache state for GL_DEPTH_WRITEMASK");
    this->ResetGLDepthMaskState();
    error = true;
  }
  ::glGetBooleanv(GL_COLOR_WRITEMASK, params);
  if (params[0] != this->CurrentState.ColorMask[0] ||
    params[1] != this->CurrentState.ColorMask[1] || params[2] != this->CurrentState.ColorMask[2] ||
    params[3] != this->CurrentState.ColorMask[3])
  {
    svtkGenericWarningMacro("Error in cache state for GL_COLOR_WRITEMASK");
    this->ResetGLColorMaskState();
    error = true;
  }
  ::glGetBooleanv(GL_BLEND, params);
  if ((params[0] != 0) != this->CurrentState.Blend)
  {
    svtkGenericWarningMacro("Error in cache state for GL_BLEND");
    this->ResetEnumState(GL_BLEND);
    error = true;
  }
  ::glGetBooleanv(GL_DEPTH_TEST, params);
  if ((params[0] != 0) != this->CurrentState.DepthTest)
  {
    svtkGenericWarningMacro("Error in cache state for GL_DEPTH_TEST");
    this->ResetEnumState(GL_DEPTH_TEST);
    error = true;
  }
  ::glGetBooleanv(GL_CULL_FACE, params);
  if ((params[0] != 0) != this->CurrentState.CullFace)
  {
    svtkGenericWarningMacro("Error in cache state for GL_CULL_FACE");
    this->ResetEnumState(GL_CULL_FACE);
    error = true;
  }
#ifdef GL_MULTISAMPLE
  ::glGetBooleanv(GL_MULTISAMPLE, params);
  if ((params[0] != 0) != this->CurrentState.MultiSample)
  {
    svtkGenericWarningMacro("Error in cache state for GL_MULTISAMPLE");
    this->ResetEnumState(GL_MULTISAMPLE);
    error = true;
  }
#endif
  ::glGetBooleanv(GL_SCISSOR_TEST, params);
  if ((params[0] != 0) != this->CurrentState.ScissorTest)
  {
    svtkGenericWarningMacro("Error in cache state for GL_SCISSOR_TEST");
    this->ResetEnumState(GL_SCISSOR_TEST);
    error = true;
  }
  ::glGetBooleanv(GL_STENCIL_TEST, params);
  if ((params[0] != 0) != this->CurrentState.StencilTest)
  {
    svtkGenericWarningMacro("Error in cache state for GL_STENCIL_TEST");
    this->ResetEnumState(GL_STENCIL_TEST);
    error = true;
  }

  GLint iparams[4];
#if defined(__APPLE__)
  // OSX systems seem to change the glViewport upon a window resize
  // under the hood, so our viewport cache cannot be trusted
  this->ResetGLViewportState();
#endif
  ::glGetIntegerv(GL_VIEWPORT, iparams);
  if (iparams[0] != this->CurrentState.Viewport[0] ||
    iparams[1] != this->CurrentState.Viewport[1] || iparams[2] != this->CurrentState.Viewport[2] ||
    iparams[3] != this->CurrentState.Viewport[3])
  {
    svtkGenericWarningMacro("Error in cache state for GL_VIEWPORT");
    this->ResetGLViewportState();
    error = true;
  }
  ::glGetIntegerv(GL_SCISSOR_BOX, iparams);
  if (iparams[0] != this->CurrentState.Scissor[0] || iparams[1] != this->CurrentState.Scissor[1] ||
    iparams[2] != this->CurrentState.Scissor[2] || iparams[3] != this->CurrentState.Scissor[3])
  {
    svtkGenericWarningMacro("Error in cache state for GL_SCISSOR_BOX");
    this->ResetGLScissorState();
    error = true;
  }
  ::glGetIntegerv(GL_CULL_FACE_MODE, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.CullFaceMode))
  {
    svtkGenericWarningMacro("Error in cache state for GL_CULL_FACE_MODE");
    this->ResetGLCullFaceState();
    error = true;
  }
  ::glGetIntegerv(GL_ACTIVE_TEXTURE, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.ActiveTexture))
  {
    svtkGenericWarningMacro("Error in cache state for GL_ACTIVE_TEXTURE");
    this->ResetGLActiveTexture();
    error = true;
  }
  ::glGetIntegerv(GL_DEPTH_FUNC, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.DepthFunc))
  {
    svtkGenericWarningMacro("Error in cache state for GL_DEPTH_FUNC");
    this->ResetGLDepthFuncState();
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_SRC_RGB, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.BlendFunc[0]))
  {
    svtkGenericWarningMacro("Error in cache state for GL_BLEND_SRC_RGB");
    this->ResetGLBlendFuncState();
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_SRC_ALPHA, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.BlendFunc[2]))
  {
    svtkGenericWarningMacro("Error in cache state for GL_BLEND_SRC_ALPHA");
    this->ResetGLBlendFuncState();
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_DST_RGB, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.BlendFunc[1]))
  {
    svtkGenericWarningMacro("Error in cache state for GL_BLEND_DST_RGB");
    this->ResetGLBlendFuncState();
    error = true;
  }
  ::glGetIntegerv(GL_BLEND_DST_ALPHA, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.BlendFunc[3]))
  {
    svtkGenericWarningMacro("Error in cache state for GL_BLEND_DST_ALPHA");
    this->ResetGLBlendFuncState();
    error = true;
  }
  ::glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.DrawBinding.GetBinding()))
  {
    svtkGenericWarningMacro("Error in cache state for GL_DRAW_FRAMEBUFFER_BINDING");
    this->ResetFramebufferBindings();
    error = true;
  }
  ::glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, iparams);
  if (iparams[0] != static_cast<int>(this->CurrentState.ReadBinding.GetBinding()))
  {
    svtkGenericWarningMacro("Error in cache state for GL_READ_FRAMEBUFFER_BINDING");
    this->ResetFramebufferBindings();
    error = true;
  }
  unsigned int sval;
#ifdef GL_DRAW_BUFFER
  ::glGetIntegerv(GL_DRAW_BUFFER, iparams);
  sval = this->CurrentState.DrawBinding.GetDrawBuffer(0);
  if (sval == GL_BACK_LEFT)
  {
    sval = GL_BACK;
  }
  if (iparams[0] == GL_BACK_LEFT)
  {
    iparams[0] = GL_BACK;
  }
  if (iparams[0] != static_cast<int>(sval))
  {
    svtkGenericWarningMacro("Error in cache state for GL_DRAW_BUFFER got "
      << iparams[0] << " expected" << this->CurrentState.DrawBinding.GetDrawBuffer(0));
    this->ResetFramebufferBindings();
    error = true;
  }
#endif
  ::glGetIntegerv(GL_READ_BUFFER, iparams);
  sval = this->CurrentState.ReadBinding.GetReadBuffer();
  // handle odd left right stuff
  if (sval == GL_BACK_LEFT)
  {
    sval = GL_BACK;
  }
  if (iparams[0] == GL_BACK_LEFT)
  {
    iparams[0] = GL_BACK;
  }
  if (iparams[0] != static_cast<int>(sval))
  {
    svtkGenericWarningMacro("Error in cache state for GL_READ_BUFFER");
    this->ResetFramebufferBindings();
    error = true;
  }

  GLfloat fparams[4];
  // note people do set this to nan
  ::glGetFloatv(GL_COLOR_CLEAR_VALUE, fparams);
  if ((!(std::isnan(fparams[0]) && std::isnan(this->CurrentState.ClearColor[0])) &&
        fparams[0] != this->CurrentState.ClearColor[0]) ||
    (!(std::isnan(fparams[1]) && std::isnan(this->CurrentState.ClearColor[1])) &&
      fparams[1] != this->CurrentState.ClearColor[1]) ||
    (!(std::isnan(fparams[2]) && std::isnan(this->CurrentState.ClearColor[2])) &&
      fparams[2] != this->CurrentState.ClearColor[2]) ||
    (!(std::isnan(fparams[3]) && std::isnan(this->CurrentState.ClearColor[3])) &&
      fparams[3] != this->CurrentState.ClearColor[3]))
  {
    svtkGenericWarningMacro("Error in cache state for GL_COLOR_CLEAR_VALUE");
    this->ResetGLClearColorState();
    error = true;
  }

  if (error)
  {
    std::string msg = svtksys::SystemInformation::GetProgramStack(0, 0);
    svtkGenericWarningMacro("at stack loc\n" << msg);
  }
}

namespace
{
bool reportOpenGLErrors(std::string& result)
{
  const int maxErrors = 16;
  unsigned int errCode[maxErrors] = { 0 };
  const char* errDesc[maxErrors] = { nullptr };

  int numErrors = svtkGetOpenGLErrors(maxErrors, errCode, errDesc);

  if (numErrors)
  {
    std::ostringstream oss;
    svtkPrintOpenGLErrors(oss, maxErrors, numErrors, errCode, errDesc);

    oss << "\n with stack trace of\n" << svtksys::SystemInformation::GetProgramStack(0, 0);
    result = oss.str();
    return true;
  }
  return false;
}

} // anon namespace

#define svtkOpenGLCheckStateMacro() this->CheckState()

#define svtkCheckOpenGLErrorsWithStack(message)                                                     \
  {                                                                                                \
    std::string _tmp;                                                                              \
    if (reportOpenGLErrors(_tmp))                                                                  \
    {                                                                                              \
      svtkGenericWarningMacro("Error " << message << _tmp);                                         \
      svtkOpenGLClearErrorMacro();                                                                  \
    }                                                                                              \
  }

#else // SVTK_REPORT_OPENGL_ERRORS

#define svtkCheckOpenGLErrorsWithStack(message)
#define svtkOpenGLCheckStateMacro()

#endif // SVTK_REPORT_OPENGL_ERRORS

//
//////////////////////////////////////////////////////////////////////////////

svtkOpenGLState::BufferBindingState::BufferBindingState()
{
  this->Framebuffer = nullptr;
  this->ReadBuffer = GL_NONE;
  this->DrawBuffers[0] = GL_BACK;
}

// bool svtkOpenGLState::BufferBindingState::operator==(
//   const BufferBindingState& a, const BufferBindingState& b)
// {
//   return a.Framebuffer == b.Framebuffer &&
// }

unsigned int svtkOpenGLState::BufferBindingState::GetBinding()
{
  if (this->Framebuffer)
  {
    return this->Framebuffer->GetFBOIndex();
  }
  return this->Binding;
}

unsigned int svtkOpenGLState::BufferBindingState::GetDrawBuffer(unsigned int val)
{
  if (this->Framebuffer)
  {
    return this->Framebuffer->GetActiveDrawBuffer(val);
  }
  return this->DrawBuffers[val];
}

unsigned int svtkOpenGLState::BufferBindingState::GetReadBuffer()
{
  if (this->Framebuffer)
  {
    return this->Framebuffer->GetActiveReadBuffer();
  }
  return this->ReadBuffer;
}

svtkOpenGLState::ScopedglDepthMask::ScopedglDepthMask(svtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->CurrentState.DepthMask;
  this->Method = &svtkOpenGLState::svtkglDepthMask;
}

svtkOpenGLState::ScopedglColorMask::ScopedglColorMask(svtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->CurrentState.ColorMask;
  this->Method = &svtkOpenGLState::ColorMask;
}

svtkOpenGLState::ScopedglDepthFunc::ScopedglDepthFunc(svtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->CurrentState.DepthFunc;
  this->Method = &svtkOpenGLState::svtkglDepthFunc;
}

svtkOpenGLState::ScopedglClearColor::ScopedglClearColor(svtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->CurrentState.ClearColor;
  this->Method = &svtkOpenGLState::ClearColor;
}

svtkOpenGLState::ScopedglScissor::ScopedglScissor(svtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->CurrentState.Scissor;
  this->Method = &svtkOpenGLState::Scissor;
}

svtkOpenGLState::ScopedglViewport::ScopedglViewport(svtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->CurrentState.Viewport;
  this->Method = &svtkOpenGLState::Viewport;
}

svtkOpenGLState::ScopedglBlendFuncSeparate::ScopedglBlendFuncSeparate(svtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->CurrentState.BlendFunc;
  this->Method = &svtkOpenGLState::BlendFuncSeparate;
}

svtkOpenGLState::ScopedglActiveTexture::ScopedglActiveTexture(svtkOpenGLState* s)
{
  this->State = s;
  this->Value = this->State->CurrentState.ActiveTexture;
  this->Method = &svtkOpenGLState::svtkglActiveTexture;
}

void svtkOpenGLState::ColorMask(std::array<GLboolean, 4> val)
{
  this->svtkglColorMask(val[0], val[1], val[2], val[3]);
}

void svtkOpenGLState::svtkglColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
  svtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.ColorMask[0] != r || this->CurrentState.ColorMask[1] != g ||
    this->CurrentState.ColorMask[2] != b || this->CurrentState.ColorMask[3] != a)
#endif
  {
    this->CurrentState.ColorMask[0] = r;
    this->CurrentState.ColorMask[1] = g;
    this->CurrentState.ColorMask[2] = b;
    this->CurrentState.ColorMask[3] = a;
    ::glColorMask(r, g, b, a);
  }

  svtkCheckOpenGLErrorsWithStack("glColorMask");
}

void svtkOpenGLState::ClearColor(std::array<GLclampf, 4> val)
{
  this->svtkglClearColor(val[0], val[1], val[2], val[3]);
}

void svtkOpenGLState::svtkglClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
  svtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.ClearColor[0] != red || this->CurrentState.ClearColor[1] != green ||
    this->CurrentState.ClearColor[2] != blue || this->CurrentState.ClearColor[3] != alpha)
#endif
  {
    this->CurrentState.ClearColor[0] = red;
    this->CurrentState.ClearColor[1] = green;
    this->CurrentState.ClearColor[2] = blue;
    this->CurrentState.ClearColor[3] = alpha;
    ::glClearColor(red, green, blue, alpha);
  }

  svtkCheckOpenGLErrorsWithStack("glClearColor");
}

void svtkOpenGLState::svtkglClearDepth(double val)
{
  svtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.ClearDepth != val)
#endif
  {
    this->CurrentState.ClearDepth = val;
#ifdef GL_ES_VERSION_3_0
    ::glClearDepthf(static_cast<GLclampf>(val));
#else
    ::glClearDepth(val);
#endif
  }
  svtkCheckOpenGLErrorsWithStack("glClearDepth");
}

void svtkOpenGLState::svtkBindFramebuffer(unsigned int target, svtkOpenGLFramebufferObject* fo)
{
  svtkOpenGLCheckStateMacro();

  if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
  {
#ifndef NO_CACHE
    if (this->CurrentState.DrawBinding.Framebuffer != fo)
#endif
    {
      this->CurrentState.DrawBinding.Binding = 0;
      this->CurrentState.DrawBinding.Framebuffer = fo;
      ::glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fo ? fo->GetFBOIndex() : 0);
    }
  }

  if (target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
  {
#ifndef NO_CACHE
    if (this->CurrentState.ReadBinding.Framebuffer != fo)
#endif
    {
      this->CurrentState.ReadBinding.Binding = 0;
      this->CurrentState.ReadBinding.Framebuffer = fo;
      ::glBindFramebuffer(GL_READ_FRAMEBUFFER, fo ? fo->GetFBOIndex() : 0);
    }
  }

  svtkCheckOpenGLErrorsWithStack("glBindFramebuffer");
}

void svtkOpenGLState::svtkglBindFramebuffer(unsigned int target, unsigned int val)
{
  svtkOpenGLCheckStateMacro();

  if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
  {
#ifndef NO_CACHE
    if (this->CurrentState.DrawBinding.Framebuffer || this->CurrentState.DrawBinding.Binding != val)
#endif
    {
      this->CurrentState.DrawBinding.Binding = val;
      this->CurrentState.DrawBinding.Framebuffer = nullptr;
      ::glBindFramebuffer(GL_DRAW_FRAMEBUFFER, val);
#ifdef GL_DRAW_BUFFER
      ::glGetIntegerv(GL_DRAW_BUFFER, (int*)&this->CurrentState.DrawBinding.DrawBuffers[0]);
#endif
    }
  }

  if (target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
  {
#ifndef NO_CACHE
    if (this->CurrentState.ReadBinding.Framebuffer || this->CurrentState.ReadBinding.Binding != val)
#endif
    {
      this->CurrentState.ReadBinding.Binding = val;
      this->CurrentState.ReadBinding.Framebuffer = nullptr;
      ::glBindFramebuffer(GL_READ_FRAMEBUFFER, val);
      ::glGetIntegerv(GL_READ_BUFFER, (int*)&this->CurrentState.ReadBinding.ReadBuffer);
    }
  }

  svtkCheckOpenGLErrorsWithStack("glBindFramebuffer");
}

void svtkOpenGLState::svtkglDrawBuffer(unsigned int val)
{
  svtkOpenGLCheckStateMacro();

  if ((this->CurrentState.DrawBinding.Framebuffer || this->CurrentState.DrawBinding.Binding) &&
    val < GL_COLOR_ATTACHMENT0 && val != GL_NONE)
  {
    // todo get rid of the && and make this always an error if FO is set
    svtkGenericWarningMacro(
      "A svtkOpenGLFramebufferObject is currently bound but a hardware draw buffer was requested.");
    std::string msg = svtksys::SystemInformation::GetProgramStack(0, 0);
    svtkGenericWarningMacro("at stack loc\n" << msg);
  }

#ifndef NO_CACHE
  if (this->CurrentState.DrawBinding.DrawBuffers[0] != val)
#endif
  {
    this->CurrentState.DrawBinding.DrawBuffers[0] = val;
    ::glDrawBuffers(1, this->CurrentState.DrawBinding.DrawBuffers);
  }

  // change all stack entries for the same framebuffer
  for (auto& se : this->DrawBindings)
  {
    if (se.Framebuffer == this->CurrentState.DrawBinding.Framebuffer &&
      se.Binding == this->CurrentState.DrawBinding.Binding)
    {
      se.DrawBuffers[0] = val;
    }
  }

  svtkCheckOpenGLErrorsWithStack("glDrawBuffer");
}

void svtkOpenGLState::svtkglDrawBuffers(unsigned int count, unsigned int* vals)
{
  svtkOpenGLCheckStateMacro();

  if (count <= 0)
  {
    return;
  }

  if ((this->CurrentState.DrawBinding.Framebuffer || this->CurrentState.DrawBinding.Binding) &&
    vals[0] < GL_COLOR_ATTACHMENT0 && vals[0] != GL_NONE)
  {
    // todo get rid of the && and make this always an error if FO is set
    svtkGenericWarningMacro(
      "A svtkOpenGLFramebufferObject is currently bound but hardware draw buffers were requested.");
  }

#ifndef NO_CACHE
  bool changed = false;
  for (int i = 0; i < static_cast<int>(count) && i < 10; ++i)
  {
    if (vals[i] != this->CurrentState.DrawBinding.DrawBuffers[i])
    {
      changed = true;
    }
  }
  if (count > 10)
  {
    changed = true;
  }
  if (changed)
#endif
  {
    for (unsigned int i = 0; i < count && i < 10; ++i)
    {
      this->CurrentState.DrawBinding.DrawBuffers[i] = vals[i];
    }
    ::glDrawBuffers(count, vals);
  }

  // change all stack entries for the same framebuffer
  for (auto& se : this->DrawBindings)
  {
    if (se.Framebuffer == this->CurrentState.DrawBinding.Framebuffer &&
      se.Binding == this->CurrentState.DrawBinding.Binding)
    {
      for (unsigned int i = 0; i < count && i < 10; ++i)
      {
        se.DrawBuffers[i] = vals[i];
      }
    }
  }

  svtkCheckOpenGLErrorsWithStack("glDrawBuffers");
}

void svtkOpenGLState::svtkDrawBuffers(
  unsigned int count, unsigned int* vals, svtkOpenGLFramebufferObject* fo)
{
  svtkOpenGLCheckStateMacro();

  if (count <= 0)
  {
    return;
  }

  if (this->CurrentState.DrawBinding.Framebuffer == nullptr ||
    (vals[0] < GL_COLOR_ATTACHMENT0 && vals[0] != GL_NONE))
  {
    svtkGenericWarningMacro(
      "A svtkOpenGLFramebufferObject is not currently bound. This method should only"
      " be called from svtkOpenGLFramebufferObject.");
  }

  if (fo != this->CurrentState.DrawBinding.Framebuffer)
  {
    svtkGenericWarningMacro(
      "Attempt to set draw buffers from a Framebuffer Object that is not bound.");
  }

#ifndef NO_CACHE
  bool changed = false;
  for (int i = 0; i < static_cast<int>(count) && i < 10; ++i)
  {
    if (vals[i] != this->CurrentState.DrawBinding.GetDrawBuffer(i))
    {
      changed = true;
    }
  }
  if (count > 10)
  {
    changed = true;
  }
  if (changed)
#endif
  {
    ::glDrawBuffers(count, vals);
  }

  svtkCheckOpenGLErrorsWithStack("glDrawBuffers");
}

void svtkOpenGLState::svtkglReadBuffer(unsigned int val)
{
  svtkOpenGLCheckStateMacro();

  if ((this->CurrentState.ReadBinding.Framebuffer || this->CurrentState.ReadBinding.Binding) &&
    val < GL_COLOR_ATTACHMENT0 && val != GL_NONE)
  {
    svtkGenericWarningMacro(
      "A svtkOpenGLFramebufferObject is currently bound but a hardware read buffer was requested.");
  }

#ifndef NO_CACHE
  if (this->CurrentState.ReadBinding.ReadBuffer != val)
#endif
  {
    this->CurrentState.ReadBinding.ReadBuffer = val;
    ::glReadBuffer(val);
  }

  // change all stack entries for the same framebuffer
  for (auto& se : this->ReadBindings)
  {
    if (se.Framebuffer == this->CurrentState.ReadBinding.Framebuffer &&
      se.Binding == this->CurrentState.ReadBinding.Binding)
    {
      se.ReadBuffer = val;
    }
  }

  svtkCheckOpenGLErrorsWithStack("glReadBuffer");
}

void svtkOpenGLState::svtkReadBuffer(unsigned int val, svtkOpenGLFramebufferObject* fo)
{
  svtkOpenGLCheckStateMacro();

  if (this->CurrentState.ReadBinding.Framebuffer == nullptr ||
    (val < GL_COLOR_ATTACHMENT0 && val != GL_NONE))
  {
    svtkGenericWarningMacro(
      "A svtkOpenGLFramebufferObject is not currently bound. This method should only"
      " be called from svtkOpenGLFramebufferObject.");
  }

  if (fo != this->CurrentState.ReadBinding.Framebuffer)
  {
    svtkGenericWarningMacro(
      "Attempt to set read buffer from a Framebuffer Object that is not bound.");
  }

#ifndef NO_CACHE
  if (this->CurrentState.ReadBinding.ReadBuffer != val)
#endif
  {
    this->CurrentState.ReadBinding.ReadBuffer = val;
    ::glReadBuffer(val);
  }

  svtkCheckOpenGLErrorsWithStack("glReadBuffer");
}

void svtkOpenGLState::svtkglDepthFunc(GLenum val)
{
  svtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.DepthFunc != val)
#endif
  {
    this->CurrentState.DepthFunc = val;
    ::glDepthFunc(val);
  }
  svtkCheckOpenGLErrorsWithStack("glDepthFunc");
}

void svtkOpenGLState::svtkglDepthMask(GLboolean val)
{
  svtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.DepthMask != val)
#endif
  {
    this->CurrentState.DepthMask = val;
    ::glDepthMask(val);
  }
  svtkCheckOpenGLErrorsWithStack("glDepthMask");
}

void svtkOpenGLState::BlendFuncSeparate(std::array<GLenum, 4> val)
{
  this->svtkglBlendFuncSeparate(val[0], val[1], val[2], val[3]);
}

void svtkOpenGLState::svtkglBlendFuncSeparate(GLenum val1, GLenum val2, GLenum val3, GLenum val4)
{
  svtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.BlendFunc[0] != val1 || this->CurrentState.BlendFunc[1] != val2 ||
    this->CurrentState.BlendFunc[2] != val3 || this->CurrentState.BlendFunc[3] != val4)
#endif
  {
    this->CurrentState.BlendFunc[0] = val1;
    this->CurrentState.BlendFunc[1] = val2;
    this->CurrentState.BlendFunc[2] = val3;
    this->CurrentState.BlendFunc[3] = val4;
    ::glBlendFuncSeparate(val1, val2, val3, val4);
  }
  svtkCheckOpenGLErrorsWithStack("glBlendFuncSeparate");
}

void svtkOpenGLState::svtkglBlendEquation(GLenum val)
{
  this->svtkglBlendEquationSeparate(val, val);
}

void svtkOpenGLState::svtkglBlendEquationSeparate(GLenum val, GLenum val2)
{
  svtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.BlendEquationValue1 != val ||
    this->CurrentState.BlendEquationValue2 != val2)
#endif
  {
    this->CurrentState.BlendEquationValue1 = val;
    this->CurrentState.BlendEquationValue2 = val2;
    ::glBlendEquationSeparate(val, val2);
  }

  svtkCheckOpenGLErrorsWithStack("glBlendEquationSeparate");
}

void svtkOpenGLState::svtkglCullFace(GLenum val)
{
  svtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.CullFaceMode != val)
#endif
  {
    this->CurrentState.CullFaceMode = val;
    ::glCullFace(val);
  }
  svtkCheckOpenGLErrorsWithStack("glCullFace");
}

void svtkOpenGLState::svtkglActiveTexture(unsigned int val)
{
  svtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.ActiveTexture != val)
#endif
  {
    this->CurrentState.ActiveTexture = val;
    ::glActiveTexture(val);
  }
  svtkCheckOpenGLErrorsWithStack("glActiveTexture");
}

void svtkOpenGLState::Viewport(std::array<GLint, 4> val)
{
  this->svtkglViewport(val[0], val[1], val[2], val[3]);
}

void svtkOpenGLState::svtkglViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
  svtkOpenGLCheckStateMacro();

#if !defined(NO_CACHE) && !defined(__APPLE__)
  if (this->CurrentState.Viewport[0] != x || this->CurrentState.Viewport[1] != y ||
    this->CurrentState.Viewport[2] != width || this->CurrentState.Viewport[3] != height)
#endif
  {
    this->CurrentState.Viewport[0] = x;
    this->CurrentState.Viewport[1] = y;
    this->CurrentState.Viewport[2] = width;
    this->CurrentState.Viewport[3] = height;
    ::glViewport(x, y, width, height);
  }

  svtkCheckOpenGLErrorsWithStack("glViewport");
}

void svtkOpenGLState::Scissor(std::array<GLint, 4> val)
{
  this->svtkglScissor(val[0], val[1], val[2], val[3]);
}

void svtkOpenGLState::svtkglScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
  svtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  if (this->CurrentState.Scissor[0] != x || this->CurrentState.Scissor[1] != y ||
    this->CurrentState.Scissor[2] != width || this->CurrentState.Scissor[3] != height)
#endif
  {
    this->CurrentState.Scissor[0] = x;
    this->CurrentState.Scissor[1] = y;
    this->CurrentState.Scissor[2] = width;
    this->CurrentState.Scissor[3] = height;
    ::glScissor(x, y, width, height);
  }
  svtkCheckOpenGLErrorsWithStack("glScissor");
}

void svtkOpenGLState::SetEnumState(GLenum cap, bool val)
{
  svtkOpenGLCheckStateMacro();

#ifndef NO_CACHE
  bool changed = false;
#else
  bool changed = true;
#endif
  switch (cap)
  {
    case GL_BLEND:
      if (this->CurrentState.Blend != val)
      {
        this->CurrentState.Blend = val;
        changed = true;
      }
      break;
    case GL_DEPTH_TEST:
      if (this->CurrentState.DepthTest != val)
      {
        this->CurrentState.DepthTest = val;
        changed = true;
      }
      break;
    case GL_CULL_FACE:
      if (this->CurrentState.CullFace != val)
      {
        this->CurrentState.CullFace = val;
        changed = true;
      }
      break;
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      if (this->CurrentState.MultiSample != val)
      {
        this->CurrentState.MultiSample = val;
        changed = true;
      }
      break;
#endif
    case GL_SCISSOR_TEST:
      if (this->CurrentState.ScissorTest != val)
      {
        this->CurrentState.ScissorTest = val;
        changed = true;
      }
      break;
    case GL_STENCIL_TEST:
      if (this->CurrentState.StencilTest != val)
      {
        this->CurrentState.StencilTest = val;
        changed = true;
      }
      break;
    default:
      changed = true;
  }

  if (!changed)
  {
    return;
  }

  if (val)
  {
    ::glEnable(cap);
  }
  else
  {
    ::glDisable(cap);
  }
  svtkCheckOpenGLErrorsWithStack("glEnable/Disable");
}

void svtkOpenGLState::ResetEnumState(GLenum cap)
{
  GLboolean params;
  ::glGetBooleanv(cap, &params);

  switch (cap)
  {
    case GL_BLEND:
      this->CurrentState.Blend = params != 0;
      break;
    case GL_DEPTH_TEST:
      this->CurrentState.DepthTest = params != 0;
      break;
    case GL_CULL_FACE:
      this->CurrentState.CullFace = params != 0;
      break;
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      this->CurrentState.MultiSample = params != 0;
      break;
#endif
    case GL_SCISSOR_TEST:
      this->CurrentState.ScissorTest = params != 0;
      break;
    case GL_STENCIL_TEST:
      this->CurrentState.StencilTest = params != 0;
      break;
    default:
      break;
  }
}

void svtkOpenGLState::svtkglEnable(GLenum cap)
{
  this->SetEnumState(cap, true);
}

// return cached value if we have it
// otherwise forward to opengl
void svtkOpenGLState::svtkglGetBooleanv(GLenum pname, GLboolean* params)
{
  svtkOpenGLCheckStateMacro();

  switch (pname)
  {
    case GL_DEPTH_WRITEMASK:
      *params = this->CurrentState.DepthMask;
      break;
    case GL_COLOR_WRITEMASK:
      params[0] = this->CurrentState.ColorMask[0];
      params[1] = this->CurrentState.ColorMask[1];
      params[2] = this->CurrentState.ColorMask[2];
      params[3] = this->CurrentState.ColorMask[3];
      break;
    case GL_BLEND:
      *params = this->CurrentState.Blend;
      break;
    case GL_DEPTH_TEST:
      *params = this->CurrentState.DepthTest;
      break;
    case GL_CULL_FACE:
      *params = this->CurrentState.CullFace;
      break;
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      *params = this->CurrentState.MultiSample;
      break;
#endif
    case GL_SCISSOR_TEST:
      *params = this->CurrentState.ScissorTest;
      break;
    case GL_STENCIL_TEST:
      *params = this->CurrentState.StencilTest;
      break;
    default:
      ::glGetBooleanv(pname, params);
  }
  svtkCheckOpenGLErrorsWithStack("glGetBoolean");
}

void svtkOpenGLState::svtkglGetIntegerv(GLenum pname, GLint* params)
{
  svtkOpenGLCheckStateMacro();

  switch (pname)
  {
    case GL_VIEWPORT:
      params[0] = this->CurrentState.Viewport[0];
      params[1] = this->CurrentState.Viewport[1];
      params[2] = this->CurrentState.Viewport[2];
      params[3] = this->CurrentState.Viewport[3];
      break;
    case GL_SCISSOR_BOX:
      params[0] = this->CurrentState.Scissor[0];
      params[1] = this->CurrentState.Scissor[1];
      params[2] = this->CurrentState.Scissor[2];
      params[3] = this->CurrentState.Scissor[3];
      break;
    case GL_CULL_FACE_MODE:
      *params = this->CurrentState.CullFaceMode;
      break;
    case GL_DEPTH_FUNC:
      *params = this->CurrentState.DepthFunc;
      break;
    case GL_BLEND_SRC_RGB:
      *params = this->CurrentState.BlendFunc[0];
      break;
    case GL_BLEND_SRC_ALPHA:
      *params = this->CurrentState.BlendFunc[2];
      break;
    case GL_BLEND_DST_RGB:
      *params = this->CurrentState.BlendFunc[1];
      break;
    case GL_BLEND_DST_ALPHA:
      *params = this->CurrentState.BlendFunc[3];
      break;
    case GL_MAX_TEXTURE_SIZE:
      *params = this->CurrentState.MaxTextureSize;
      break;
    case GL_MAJOR_VERSION:
      *params = this->CurrentState.MajorVersion;
      break;
    case GL_MINOR_VERSION:
      *params = this->CurrentState.MinorVersion;
      break;
    default:
      ::glGetIntegerv(pname, params);
  }

  svtkCheckOpenGLErrorsWithStack("glGetInteger");
}

#ifdef GL_ES_VERSION_3_0
void svtkOpenGLState::svtkglGetDoublev(GLenum pname, double*)
{
  svtkGenericWarningMacro("glGetDouble not supported on OpenGL ES, requested: " << pname);
}
#else
void svtkOpenGLState::svtkglGetDoublev(GLenum pname, double* params)
{
  svtkOpenGLCheckStateMacro();
  ::glGetDoublev(pname, params);
  svtkCheckOpenGLErrorsWithStack("glGetDouble");
}
#endif

void svtkOpenGLState::svtkglGetFloatv(GLenum pname, GLfloat* params)
{
  svtkOpenGLCheckStateMacro();

  switch (pname)
  {
    case GL_COLOR_CLEAR_VALUE:
      params[0] = this->CurrentState.ClearColor[0];
      params[1] = this->CurrentState.ClearColor[1];
      params[2] = this->CurrentState.ClearColor[2];
      params[3] = this->CurrentState.ClearColor[3];
      break;
    default:
      ::glGetFloatv(pname, params);
  }
  svtkCheckOpenGLErrorsWithStack("glGetFloat");
}

void svtkOpenGLState::GetBlendFuncState(int* v)
{
  v[0] = this->CurrentState.BlendFunc[0];
  v[1] = this->CurrentState.BlendFunc[1];
  v[2] = this->CurrentState.BlendFunc[2];
  v[3] = this->CurrentState.BlendFunc[3];
}

bool svtkOpenGLState::GetEnumState(GLenum cap)
{
  svtkOpenGLCheckStateMacro();

  switch (cap)
  {
    case GL_BLEND:
      return this->CurrentState.Blend;
    case GL_DEPTH_TEST:
      return this->CurrentState.DepthTest;
    case GL_CULL_FACE:
      return this->CurrentState.CullFace;
#ifdef GL_MULTISAMPLE
    case GL_MULTISAMPLE:
      return this->CurrentState.MultiSample;
#endif
    case GL_SCISSOR_TEST:
      return this->CurrentState.ScissorTest;
    case GL_STENCIL_TEST:
      return this->CurrentState.StencilTest;
    default:
      svtkGenericWarningMacro("Bad request for enum status");
  }
  return false;
}

void svtkOpenGLState::svtkglDisable(GLenum cap)
{
  this->SetEnumState(cap, false);
}

// make the hardware openglstate match the
// state ivars
void svtkOpenGLState::Initialize(svtkOpenGLRenderWindow*)
{
  this->TextureUnitManager->Initialize();
  this->InitializeTextureInternalFormats();

  this->CurrentState.Blend ? ::glEnable(GL_BLEND) : ::glDisable(GL_BLEND);
  this->CurrentState.DepthTest ? ::glEnable(GL_DEPTH_TEST) : ::glDisable(GL_DEPTH_TEST);
  this->CurrentState.StencilTest ? ::glEnable(GL_STENCIL_TEST) : ::glDisable(GL_STENCIL_TEST);
  this->CurrentState.ScissorTest ? ::glEnable(GL_SCISSOR_TEST) : ::glDisable(GL_SCISSOR_TEST);
  this->CurrentState.CullFace ? ::glEnable(GL_CULL_FACE) : ::glDisable(GL_CULL_FACE);

#ifdef GL_MULTISAMPLE
  this->CurrentState.MultiSample = glIsEnabled(GL_MULTISAMPLE) == GL_TRUE;
#endif

  // initialize blending for transparency
  ::glBlendFuncSeparate(this->CurrentState.BlendFunc[0], this->CurrentState.BlendFunc[1],
    this->CurrentState.BlendFunc[2], this->CurrentState.BlendFunc[3]);

  ::glClearColor(this->CurrentState.ClearColor[0], this->CurrentState.ClearColor[1],
    this->CurrentState.ClearColor[2], this->CurrentState.ClearColor[3]);

  ::glColorMask(this->CurrentState.ColorMask[0], this->CurrentState.ColorMask[1],
    this->CurrentState.ColorMask[2], this->CurrentState.ColorMask[3]);

  ::glDepthFunc(this->CurrentState.DepthFunc);

#ifdef GL_ES_VERSION_3_0
  ::glClearDepthf(this->CurrentState.ClearDepth);
#else
  ::glClearDepth(this->CurrentState.ClearDepth);
#endif

  ::glDepthMask(this->CurrentState.DepthMask);

  ::glViewport(this->CurrentState.Viewport[0], this->CurrentState.Viewport[1],
    this->CurrentState.Viewport[2], this->CurrentState.Viewport[3]);

  ::glScissor(this->CurrentState.Scissor[0], this->CurrentState.Scissor[1],
    this->CurrentState.Scissor[2], this->CurrentState.Scissor[3]);

  ::glCullFace(this->CurrentState.CullFaceMode);

  ::glBlendEquationSeparate(
    this->CurrentState.BlendEquationValue1, this->CurrentState.BlendEquationValue2);

  // strictly query values below here
  ::glGetIntegerv(GL_MAX_TEXTURE_SIZE, &this->CurrentState.MaxTextureSize);
  ::glGetIntegerv(GL_MAJOR_VERSION, &this->CurrentState.MajorVersion);
  ::glGetIntegerv(GL_MINOR_VERSION, &this->CurrentState.MinorVersion);

  ::glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->CurrentState.DrawBinding.GetBinding());
  ::glBindFramebuffer(GL_READ_FRAMEBUFFER, this->CurrentState.ReadBinding.GetBinding());
  unsigned int vals[1];
  vals[0] = this->CurrentState.DrawBinding.GetDrawBuffer(0);
  ::glDrawBuffers(1, vals);
#ifdef GL_DRAW_BUFFER
  ::glGetIntegerv(GL_DRAW_BUFFER, (int*)&this->CurrentState.DrawBinding.DrawBuffers[0]);
#endif
  ::glReadBuffer(this->CurrentState.ReadBinding.GetReadBuffer());
  ::glGetIntegerv(GL_READ_BUFFER, (int*)&this->CurrentState.ReadBinding.ReadBuffer);
}

void svtkOpenGLState::ResetFramebufferBindings()
{
  ::glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (int*)&this->CurrentState.DrawBinding.Binding);
#ifdef GL_DRAW_BUFFER
  ::glGetIntegerv(GL_DRAW_BUFFER, (int*)&this->CurrentState.DrawBinding.DrawBuffers[0]);
#endif

  ::glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (int*)&this->CurrentState.ReadBinding.Binding);
  ::glGetIntegerv(GL_READ_BUFFER, (int*)&this->CurrentState.ReadBinding.ReadBuffer);
}

void svtkOpenGLState::ResetGLClearColorState()
{
  GLfloat fparams[4];
  ::glGetFloatv(GL_COLOR_CLEAR_VALUE, fparams);
  this->CurrentState.ClearColor[0] = fparams[0];
  this->CurrentState.ClearColor[1] = fparams[1];
  this->CurrentState.ClearColor[2] = fparams[2];
  this->CurrentState.ClearColor[3] = fparams[3];
}

void svtkOpenGLState::ResetGLClearDepthState()
{
  GLfloat fparams;
  ::glGetFloatv(GL_DEPTH_CLEAR_VALUE, &fparams);
  this->CurrentState.ClearDepth = fparams;
}

void svtkOpenGLState::ResetGLDepthFuncState()
{
  GLint iparams;
  ::glGetIntegerv(GL_DEPTH_FUNC, &iparams);
  this->CurrentState.DepthFunc = static_cast<unsigned int>(iparams);
}

void svtkOpenGLState::ResetGLDepthMaskState()
{
  GLboolean params;
  ::glGetBooleanv(GL_DEPTH_WRITEMASK, &params);
  this->CurrentState.DepthMask = params;
}

void svtkOpenGLState::ResetGLColorMaskState()
{
  GLboolean params[4];
  ::glGetBooleanv(GL_COLOR_WRITEMASK, params);
  this->CurrentState.ColorMask[0] = params[0];
  this->CurrentState.ColorMask[1] = params[1];
  this->CurrentState.ColorMask[2] = params[2];
  this->CurrentState.ColorMask[3] = params[3];
}

void svtkOpenGLState::ResetGLViewportState()
{
  GLint iparams[4];
  ::glGetIntegerv(GL_VIEWPORT, iparams);
  this->CurrentState.Viewport[0] = iparams[0];
  this->CurrentState.Viewport[1] = iparams[1];
  this->CurrentState.Viewport[2] = iparams[2];
  this->CurrentState.Viewport[3] = iparams[3];
}

void svtkOpenGLState::ResetGLScissorState()
{
  GLint iparams[4];
  ::glGetIntegerv(GL_SCISSOR_BOX, iparams);
  this->CurrentState.Scissor[0] = iparams[0];
  this->CurrentState.Scissor[1] = iparams[1];
  this->CurrentState.Scissor[2] = iparams[2];
  this->CurrentState.Scissor[3] = iparams[3];
}

void svtkOpenGLState::ResetGLBlendFuncState()
{
  GLint iparams;
  ::glGetIntegerv(GL_BLEND_SRC_RGB, &iparams);
  this->CurrentState.BlendFunc[0] = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_SRC_ALPHA, &iparams);
  this->CurrentState.BlendFunc[2] = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_DST_RGB, &iparams);
  this->CurrentState.BlendFunc[1] = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_DST_ALPHA, &iparams);
  this->CurrentState.BlendFunc[3] = static_cast<unsigned int>(iparams);
}

void svtkOpenGLState::ResetGLBlendEquationState()
{
  GLint iparams;
  ::glGetIntegerv(GL_BLEND_EQUATION_RGB, &iparams);
  this->CurrentState.BlendEquationValue1 = static_cast<unsigned int>(iparams);
  ::glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &iparams);
  this->CurrentState.BlendEquationValue2 = static_cast<unsigned int>(iparams);
}

void svtkOpenGLState::ResetGLCullFaceState()
{
  GLint iparams;
  ::glGetIntegerv(GL_CULL_FACE_MODE, &iparams);
  this->CurrentState.CullFaceMode = static_cast<unsigned int>(iparams);
}

void svtkOpenGLState::ResetGLActiveTexture()
{
  GLint iparams;
  ::glGetIntegerv(GL_ACTIVE_TEXTURE, &iparams);
  this->CurrentState.ActiveTexture = static_cast<unsigned int>(iparams);
}

void svtkOpenGLState::svtkglClear(GLbitfield val)
{
  ::glClear(val);
}

// ----------------------------------------------------------------------------
// Description:
// Returns its texture unit manager object.
svtkTextureUnitManager* svtkOpenGLState::GetTextureUnitManager()
{
  return this->TextureUnitManager;
}

void svtkOpenGLState::SetTextureUnitManager(svtkTextureUnitManager* tum)
{
  if (this->TextureUnitManager == tum)
  {
    return;
  }
  if (tum)
  {
    tum->Register(nullptr);
  }
  if (this->TextureUnitManager)
  {
    this->TextureUnitManager->Delete();
  }
  this->TextureUnitManager = tum;
}

void svtkOpenGLState::ActivateTexture(svtkTextureObject* texture)
{
  // Only add if it isn't already there
  typedef std::map<const svtkTextureObject*, int>::const_iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found == this->TextureResourceIds.end())
  {
    int activeUnit = this->GetTextureUnitManager()->Allocate();
    if (activeUnit < 0)
    {
      svtkGenericWarningMacro("Hardware does not support the number of textures defined.");
      return;
    }
    this->TextureResourceIds.insert(std::make_pair(texture, activeUnit));
    this->svtkglActiveTexture(GL_TEXTURE0 + activeUnit);
  }
  else
  {
    this->svtkglActiveTexture(GL_TEXTURE0 + found->second);
  }
}

void svtkOpenGLState::DeactivateTexture(svtkTextureObject* texture)
{
  // Only deactivate if it isn't already there
  typedef std::map<const svtkTextureObject*, int>::iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found != this->TextureResourceIds.end())
  {
    this->GetTextureUnitManager()->Free(found->second);
    this->TextureResourceIds.erase(found);
  }
}

int svtkOpenGLState::GetTextureUnitForTexture(svtkTextureObject* texture)
{
  // Only deactivate if it isn't already there
  typedef std::map<const svtkTextureObject*, int>::const_iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found != this->TextureResourceIds.end())
  {
    return found->second;
  }

  return -1;
}

void svtkOpenGLState::VerifyNoActiveTextures()
{
  if (!this->TextureResourceIds.empty())
  {
    svtkGenericWarningMacro("There are still active textures when there should not be.");
    typedef std::map<const svtkTextureObject*, int>::const_iterator TRIter;
    TRIter found = this->TextureResourceIds.begin();
    for (; found != this->TextureResourceIds.end(); ++found)
    {
      svtkGenericWarningMacro(
        "Leaked for texture object: " << const_cast<svtkTextureObject*>(found->first));
    }
  }
}

svtkStandardNewMacro(svtkOpenGLState);

svtkCxxSetObjectMacro(svtkOpenGLState, VBOCache, svtkOpenGLVertexBufferObjectCache);

// initialize all state values. This is important so that in
// ::Initialize we can just set the state to the current
// values (knowing that they are set). The reason we want
// Initialize to set to the current values is to reduce
// OpenGL churn in cases where application call Initialize
// often without really changing many of the values. For example
//
// viewport(0,0,100,100);
// Initialize(0,0,1,1); // using hardcoded initialization
// viewport(0,0,100,100);
//
// versus
//
// viewport(0,0,100,100);
// Initialize(0,0,100,100); // using last value
// viewport(0,0,100,100); // cache will skip this line
//
// Using current values avoids extra state changes when
// not required.
//
svtkOpenGLState::svtkOpenGLState()
{
  this->ShaderCache = svtkOpenGLShaderCache::New();
  this->VBOCache = svtkOpenGLVertexBufferObjectCache::New();

  this->TextureUnitManager = svtkTextureUnitManager::New();

  this->CurrentState.Blend = true;
  this->CurrentState.DepthTest = true;
  this->CurrentState.StencilTest = false;
  this->CurrentState.ScissorTest = true;
  this->CurrentState.CullFace = false;

  this->CurrentState.MultiSample = false;

  // initialize blending for transparency
  this->CurrentState.BlendFunc[0] = GL_SRC_ALPHA;
  this->CurrentState.BlendFunc[1] = GL_ONE_MINUS_SRC_ALPHA;
  this->CurrentState.BlendFunc[2] = GL_ONE;
  this->CurrentState.BlendFunc[3] = GL_ONE_MINUS_SRC_ALPHA;

  this->CurrentState.ClearColor[0] = 0.0;
  this->CurrentState.ClearColor[1] = 0.0;
  this->CurrentState.ClearColor[2] = 0.0;
  this->CurrentState.ClearColor[3] = 0.0;

  this->CurrentState.ColorMask[0] = GL_TRUE;
  this->CurrentState.ColorMask[1] = GL_TRUE;
  this->CurrentState.ColorMask[2] = GL_TRUE;
  this->CurrentState.ColorMask[3] = GL_TRUE;

  this->CurrentState.DepthFunc = GL_LEQUAL;

  this->CurrentState.ClearDepth = 1.0;

  this->CurrentState.DepthMask = GL_TRUE;

  this->CurrentState.Viewport[0] = 0;
  this->CurrentState.Viewport[1] = 0;
  this->CurrentState.Viewport[2] = 1;
  this->CurrentState.Viewport[3] = 1;

  this->CurrentState.Scissor[0] = 0;
  this->CurrentState.Scissor[1] = 0;
  this->CurrentState.Scissor[2] = 1;
  this->CurrentState.Scissor[3] = 1;

  this->CurrentState.CullFaceMode = GL_BACK;
  this->CurrentState.ActiveTexture = GL_TEXTURE0;

  this->CurrentState.BlendEquationValue1 = GL_FUNC_ADD;
  this->CurrentState.BlendEquationValue2 = GL_FUNC_ADD;

  this->CurrentState.DrawBinding.Binding = 0;
  this->CurrentState.ReadBinding.Binding = 0;
  this->CurrentState.DrawBinding.DrawBuffers[0] = GL_BACK_LEFT;
  for (int i = 1; i < 10; ++i)
  {
    this->CurrentState.DrawBinding.DrawBuffers[i] = GL_NONE;
  }
  this->CurrentState.ReadBinding.ReadBuffer = GL_BACK_LEFT;
}

svtkOpenGLState::~svtkOpenGLState()
{
  this->TextureResourceIds.clear();
  this->SetTextureUnitManager(nullptr);
  this->VBOCache->Delete();
  this->ShaderCache->Delete();
}

void svtkOpenGLState::PushDrawFramebufferBinding()
{
  this->DrawBindings.push_back(this->CurrentState.DrawBinding);
}

void svtkOpenGLState::PushReadFramebufferBinding()
{
  this->ReadBindings.push_back(this->CurrentState.ReadBinding);
}

void svtkOpenGLState::PopDrawFramebufferBinding()
{
  if (this->DrawBindings.size())
  {
    BufferBindingState& bbs = this->DrawBindings.back();
    ::glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bbs.GetBinding());
    this->CurrentState.DrawBinding = bbs;
    this->DrawBindings.pop_back();
  }
  else
  {
    svtkGenericWarningMacro("Attempt to pop framebuffer beyond beginning of the stack.");
    abort();
  }
}

void svtkOpenGLState::PopReadFramebufferBinding()
{
  if (this->ReadBindings.size())
  {
    BufferBindingState& bbs = this->ReadBindings.back();
    ::glBindFramebuffer(GL_READ_FRAMEBUFFER, bbs.GetBinding());
    this->CurrentState.ReadBinding = bbs;
    this->ReadBindings.pop_back();
  }
  else
  {
    svtkGenericWarningMacro("Attempt to pop framebuffer beyond beginning of the stack.");
    abort();
  }
}

int svtkOpenGLState::GetDefaultTextureInternalFormat(
  int svtktype, int numComponents, bool needInt, bool needFloat, bool needSRGB)
{
  // 0 = none
  // 1 = float
  // 2 = int
  if (svtktype >= SVTK_UNICODE_STRING)
  {
    return 0;
  }
  if (needInt)
  {
    return this->TextureInternalFormats[svtktype][2][numComponents];
  }
  if (needFloat)
  {
    return this->TextureInternalFormats[svtktype][1][numComponents];
  }
  int result = this->TextureInternalFormats[svtktype][0][numComponents];
  if (needSRGB)
  {
    switch (result)
    {
#ifdef GL_ES_VERSION_3_0
      case GL_RGB:
        result = GL_SRGB8;
        break;
      case GL_RGBA:
        result = GL_SRGB8_ALPHA8;
        break;
#else
      case GL_RGB:
        result = GL_SRGB;
        break;
      case GL_RGBA:
        result = GL_SRGB_ALPHA;
        break;
#endif
      case GL_RGB8:
        result = GL_SRGB8;
        break;
      case GL_RGBA8:
        result = GL_SRGB8_ALPHA8;
        break;
      default:
        break;
    }
  }
  return result;
}

void svtkOpenGLState::InitializeTextureInternalFormats()
{
  // 0 = none
  // 1 = float
  // 2 = int

  // initialize to zero
  for (int dtype = 0; dtype < SVTK_UNICODE_STRING; dtype++)
  {
    for (int ctype = 0; ctype < 3; ctype++)
    {
      for (int comp = 0; comp <= 4; comp++)
      {
        this->TextureInternalFormats[dtype][ctype][comp] = 0;
      }
    }
  }

  this->TextureInternalFormats[SVTK_VOID][0][1] = GL_DEPTH_COMPONENT;

#ifdef GL_R8
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][0][1] = GL_R8;
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][0][2] = GL_RG8;
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][0][3] = GL_RGB8;
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][0][4] = GL_RGBA8;
#else
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][0][1] = GL_LUMINANCE;
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][0][2] = GL_LUMINANCE_ALPHA;
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][0][3] = GL_RGB;
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][0][4] = GL_RGBA;
#endif

#ifdef GL_R16
  this->TextureInternalFormats[SVTK_UNSIGNED_SHORT][0][1] = GL_R16;
  this->TextureInternalFormats[SVTK_UNSIGNED_SHORT][0][2] = GL_RG16;
  this->TextureInternalFormats[SVTK_UNSIGNED_SHORT][0][3] = GL_RGB16;
  this->TextureInternalFormats[SVTK_UNSIGNED_SHORT][0][4] = GL_RGBA16;
#endif

#ifdef GL_R8_SNORM
  this->TextureInternalFormats[SVTK_SIGNED_CHAR][0][1] = GL_R8_SNORM;
  this->TextureInternalFormats[SVTK_SIGNED_CHAR][0][2] = GL_RG8_SNORM;
  this->TextureInternalFormats[SVTK_SIGNED_CHAR][0][3] = GL_RGB8_SNORM;
  this->TextureInternalFormats[SVTK_SIGNED_CHAR][0][4] = GL_RGBA8_SNORM;
#endif

#ifdef GL_R16_SNORM
  this->TextureInternalFormats[SVTK_SHORT][0][1] = GL_R16_SNORM;
  this->TextureInternalFormats[SVTK_SHORT][0][2] = GL_RG16_SNORM;
  this->TextureInternalFormats[SVTK_SHORT][0][3] = GL_RGB16_SNORM;
  this->TextureInternalFormats[SVTK_SHORT][0][4] = GL_RGBA16_SNORM;
#endif

#ifdef GL_R8I
  this->TextureInternalFormats[SVTK_SIGNED_CHAR][2][1] = GL_R8I;
  this->TextureInternalFormats[SVTK_SIGNED_CHAR][2][2] = GL_RG8I;
  this->TextureInternalFormats[SVTK_SIGNED_CHAR][2][3] = GL_RGB8I;
  this->TextureInternalFormats[SVTK_SIGNED_CHAR][2][4] = GL_RGBA8I;
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][2][1] = GL_R8UI;
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][2][2] = GL_RG8UI;
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][2][3] = GL_RGB8UI;
  this->TextureInternalFormats[SVTK_UNSIGNED_CHAR][2][4] = GL_RGBA8UI;

  this->TextureInternalFormats[SVTK_SHORT][2][1] = GL_R16I;
  this->TextureInternalFormats[SVTK_SHORT][2][2] = GL_RG16I;
  this->TextureInternalFormats[SVTK_SHORT][2][3] = GL_RGB16I;
  this->TextureInternalFormats[SVTK_SHORT][2][4] = GL_RGBA16I;
  this->TextureInternalFormats[SVTK_UNSIGNED_SHORT][2][1] = GL_R16UI;
  this->TextureInternalFormats[SVTK_UNSIGNED_SHORT][2][2] = GL_RG16UI;
  this->TextureInternalFormats[SVTK_UNSIGNED_SHORT][2][3] = GL_RGB16UI;
  this->TextureInternalFormats[SVTK_UNSIGNED_SHORT][2][4] = GL_RGBA16UI;

  this->TextureInternalFormats[SVTK_INT][2][1] = GL_R32I;
  this->TextureInternalFormats[SVTK_INT][2][2] = GL_RG32I;
  this->TextureInternalFormats[SVTK_INT][2][3] = GL_RGB32I;
  this->TextureInternalFormats[SVTK_INT][2][4] = GL_RGBA32I;
  this->TextureInternalFormats[SVTK_UNSIGNED_INT][2][1] = GL_R32UI;
  this->TextureInternalFormats[SVTK_UNSIGNED_INT][2][2] = GL_RG32UI;
  this->TextureInternalFormats[SVTK_UNSIGNED_INT][2][3] = GL_RGB32UI;
  this->TextureInternalFormats[SVTK_UNSIGNED_INT][2][4] = GL_RGBA32UI;
#endif

  // on mesa we may not have float textures even though we think we do
  // this is due to Mesa being impacted by a patent issue with SGI
  // that is due to expire in the US in summer 2018
#ifndef GL_ES_VERSION_3_0
  const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  if (glVersion && strstr(glVersion, "Mesa") != nullptr && !GLEW_ARB_texture_float)
  {
    // mesa without float support cannot even use
    // uchar textures with underlying float data
    // so pretty much anything with float data
    // is out of luck so return
    return;
  }
#endif

#ifdef GL_R32F
  this->TextureInternalFormats[SVTK_FLOAT][1][1] = GL_R32F;
  this->TextureInternalFormats[SVTK_FLOAT][1][2] = GL_RG32F;
  this->TextureInternalFormats[SVTK_FLOAT][1][3] = GL_RGB32F;
  this->TextureInternalFormats[SVTK_FLOAT][1][4] = GL_RGBA32F;

  this->TextureInternalFormats[SVTK_SHORT][1][1] = GL_R32F;
  this->TextureInternalFormats[SVTK_SHORT][1][2] = GL_RG32F;
  this->TextureInternalFormats[SVTK_SHORT][1][3] = GL_RGB32F;
  this->TextureInternalFormats[SVTK_SHORT][1][4] = GL_RGBA32F;
#endif
}
