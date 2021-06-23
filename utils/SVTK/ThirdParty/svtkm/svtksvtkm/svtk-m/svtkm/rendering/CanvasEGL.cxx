//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/CanvasEGL.h>

#include <svtkm/Types.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/rendering/CanvasGL.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/internal/OpenGLHeaders.h>

#include <EGL/egl.h>
//#include <GL/gl.h>

namespace svtkm
{
namespace rendering
{

namespace detail
{

struct CanvasEGLInternals
{
  EGLContext Context;
  EGLDisplay Display;
  EGLSurface Surface;
};

} // namespace detail

CanvasEGL::CanvasEGL(svtkm::Id width, svtkm::Id height)
  : CanvasGL(width, height)
  , Internals(new detail::CanvasEGLInternals)
{
  this->Internals->Context = nullptr;
  this->ResizeBuffers(width, height);
}

CanvasEGL::~CanvasEGL()
{
}

void CanvasEGL::Initialize()
{
  this->Internals->Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (!(this->Internals->Display))
  {
    throw svtkm::cont::ErrorBadValue("Failed to get EGL display");
  }
  EGLint major, minor;
  if (!(eglInitialize(this->Internals->Display, &major, &minor)))
  {
    throw svtkm::cont::ErrorBadValue("Failed to initialize EGL display");
  }

  const EGLint cfgAttrs[] = { EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_RED_SIZE,
                              8,
                              EGL_DEPTH_SIZE,
                              8,
                              EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_BIT,
                              EGL_NONE };

  EGLint nCfgs;
  EGLConfig cfg;
  if (!(eglChooseConfig(this->Internals->Display, cfgAttrs, &cfg, 1, &nCfgs)) || (nCfgs == 0))
  {
    throw svtkm::cont::ErrorBadValue("Failed to get EGL config");
  }

  const EGLint pbAttrs[] = {
    EGL_WIDTH,  static_cast<EGLint>(this->GetWidth()),
    EGL_HEIGHT, static_cast<EGLint>(this->GetHeight()),
    EGL_NONE,
  };

  this->Internals->Surface = eglCreatePbufferSurface(this->Internals->Display, cfg, pbAttrs);
  if (!this->Internals->Surface)
  {
    throw svtkm::cont::ErrorBadValue("Failed to create EGL PBuffer surface");
  }
  eglBindAPI(EGL_OPENGL_API);
  this->Internals->Context =
    eglCreateContext(this->Internals->Display, cfg, EGL_NO_CONTEXT, nullptr);
  if (!this->Internals->Context)
  {
    throw svtkm::cont::ErrorBadValue("Failed to create EGL context");
  }
  if (!(eglMakeCurrent(this->Internals->Display,
                       this->Internals->Surface,
                       this->Internals->Surface,
                       this->Internals->Context)))
  {
    throw svtkm::cont::ErrorBadValue("Failed to create EGL context current");
  }
}

void CanvasEGL::Activate()
{
  glEnable(GL_DEPTH_TEST);
}

svtkm::rendering::Canvas* CanvasEGL::NewCopy() const
{
  return new svtkm::rendering::CanvasEGL(*this);
}
}
} // namespace svtkm::rendering
