//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/CanvasOSMesa.h>

#include <svtkm/Types.h>
#include <svtkm/rendering/CanvasGL.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/internal/OpenGLHeaders.h>
#ifndef GLAPI
#define GLAPI extern
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifndef APIENTRY
#define APIENTRY GLAPIENTRY
#endif
#include <GL/osmesa.h>

namespace svtkm
{
namespace rendering
{

namespace detail
{

struct CanvasOSMesaInternals
{
  OSMesaContext Context;
};

} // namespace detail

CanvasOSMesa::CanvasOSMesa(svtkm::Id width, svtkm::Id height)
  : CanvasGL(width, height)
  , Internals(new detail::CanvasOSMesaInternals)
{
  this->Internals->Context = nullptr;
  this->ResizeBuffers(width, height);
}

CanvasOSMesa::~CanvasOSMesa()
{
}

void CanvasOSMesa::Initialize()
{
  this->Internals->Context = OSMesaCreateContextExt(OSMESA_RGBA, 32, 0, 0, nullptr);

  if (!this->Internals->Context)
  {
    throw svtkm::cont::ErrorBadValue("OSMesa context creation failed.");
  }
  svtkm::Vec4f_32* colorBuffer = this->GetColorBuffer().GetStorage().GetArray();
  if (!OSMesaMakeCurrent(this->Internals->Context,
                         reinterpret_cast<svtkm::Float32*>(colorBuffer),
                         GL_FLOAT,
                         static_cast<GLsizei>(this->GetWidth()),
                         static_cast<GLsizei>(this->GetHeight())))
  {
    throw svtkm::cont::ErrorBadValue("OSMesa context activation failed.");
  }
}

void CanvasOSMesa::RefreshColorBuffer() const
{
  // Override superclass because our OSMesa implementation renders right
  // to the color buffer.
}

void CanvasOSMesa::Activate()
{
  glEnable(GL_DEPTH_TEST);
}

void CanvasOSMesa::Finish()
{
  this->CanvasGL::Finish();

// This is disabled because it is handled in RefreshDepthBuffer
#if 0
  //Copy zbuff into floating point array.
  unsigned int *raw_zbuff;
  int zbytes, w, h;
  GLboolean ret;
  ret = OSMesaGetDepthBuffer(this->Internals->Context, &w, &h, &zbytes, (void**)&raw_zbuff);
  if (!ret ||
      static_cast<svtkm::Id>(w)!=this->GetWidth() ||
      static_cast<svtkm::Id>(h)!=this->GetHeight())
  {
    throw svtkm::cont::ErrorBadValue("Wrong width/height in ZBuffer");
  }
  svtkm::cont::ArrayHandle<svtkm::Float32>::PortalControl depthPortal =
      this->GetDepthBuffer().GetPortalControl();
  svtkm::Id npixels = this->GetWidth()*this->GetHeight();
  for (svtkm::Id i=0; i<npixels; i++)
  for (std::size_t i=0; i<npixels; i++)
  {
    depthPortal.Set(i, float(raw_zbuff[i]) / float(UINT_MAX));
  }
#endif
}

svtkm::rendering::Canvas* CanvasOSMesa::NewCopy() const
{
  return new svtkm::rendering::CanvasOSMesa(*this);
}
}
} // namespace svtkm::rendering
