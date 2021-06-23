//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/LineRenderer.h>

#include <svtkm/Transform3D.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace rendering
{

LineRenderer::LineRenderer(const svtkm::rendering::Canvas* canvas,
                           svtkm::Matrix<svtkm::Float32, 4, 4> transform)
  : Canvas(canvas)
  , Transform(transform)
{
}

void LineRenderer::RenderLine(const svtkm::Vec2f_64& point0,
                              const svtkm::Vec2f_64& point1,
                              svtkm::Float32 lineWidth,
                              const svtkm::rendering::Color& color)
{
  RenderLine(svtkm::make_Vec(point0[0], point0[1], 0.0),
             svtkm::make_Vec(point1[0], point1[1], 0.0),
             lineWidth,
             color);
}

void LineRenderer::RenderLine(const svtkm::Vec3f_64& point0,
                              const svtkm::Vec3f_64& point1,
                              svtkm::Float32 svtkmNotUsed(lineWidth),
                              const svtkm::rendering::Color& color)
{
  svtkm::Vec3f_32 p0 = TransformPoint(point0);
  svtkm::Vec3f_32 p1 = TransformPoint(point1);

  svtkm::Id x0 = static_cast<svtkm::Id>(svtkm::Round(p0[0]));
  svtkm::Id y0 = static_cast<svtkm::Id>(svtkm::Round(p0[1]));
  svtkm::Float32 z0 = static_cast<svtkm::Float32>(p0[2]);
  svtkm::Id x1 = static_cast<svtkm::Id>(svtkm::Round(p1[0]));
  svtkm::Id y1 = static_cast<svtkm::Id>(svtkm::Round(p1[1]));
  svtkm::Float32 z1 = static_cast<svtkm::Float32>(p1[2]);
  svtkm::Id dx = svtkm::Abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  svtkm::Id dy = -svtkm::Abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  svtkm::Id err = dx + dy, err2 = 0;
  auto colorPortal =
    svtkm::rendering::Canvas::ColorBufferType(Canvas->GetColorBuffer()).GetPortalControl();
  auto depthPortal =
    svtkm::rendering::Canvas::DepthBufferType(Canvas->GetDepthBuffer()).GetPortalControl();
  svtkm::Vec4f_32 colorC = color.Components;

  while (x0 >= 0 && x0 < Canvas->GetWidth() && y0 >= 0 && y0 < Canvas->GetHeight())
  {
    svtkm::Float32 t = (dx == 0) ? 1.0f : (static_cast<svtkm::Float32>(x0) - p0[0]) / (p1[0] - p0[0]);
    t = svtkm::Min(1.f, svtkm::Max(0.f, t));
    svtkm::Float32 z = svtkm::Lerp(z0, z1, t);
    svtkm::Id index = y0 * Canvas->GetWidth() + x0;
    svtkm::Vec4f_32 currentColor = colorPortal.Get(index);
    svtkm::Float32 currentZ = depthPortal.Get(index);
    bool blend = currentColor[3] < 1.f && z > currentZ;
    if (currentZ > z || blend)
    {
      svtkm::Vec4f_32 writeColor = colorC;
      svtkm::Float32 depth = z;

      if (blend)
      {
        // If there is any transparency, all alphas
        // have been pre-mulitplied
        svtkm::Float32 alpha = (1.f - currentColor[3]);
        writeColor[0] = currentColor[0] + colorC[0] * alpha;
        writeColor[1] = currentColor[1] + colorC[1] * alpha;
        writeColor[2] = currentColor[2] + colorC[2] * alpha;
        writeColor[3] = 1.f * alpha + currentColor[3]; // we are always drawing opaque lines
        // keep the current z. Line z interpolation is not accurate
        depth = currentZ;
      }

      depthPortal.Set(index, depth);
      colorPortal.Set(index, writeColor);
    }

    if (x0 == x1 && y0 == y1)
    {
      break;
    }
    err2 = err * 2;
    if (err2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (err2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

svtkm::Vec3f_32 LineRenderer::TransformPoint(const svtkm::Vec3f_64& point) const
{
  svtkm::Vec4f_32 temp(static_cast<svtkm::Float32>(point[0]),
                      static_cast<svtkm::Float32>(point[1]),
                      static_cast<svtkm::Float32>(point[2]),
                      1.0f);
  temp = svtkm::MatrixMultiply(Transform, temp);
  svtkm::Vec3f_32 p;
  for (svtkm::IdComponent i = 0; i < 3; ++i)
  {
    p[i] = static_cast<svtkm::Float32>(temp[i] / temp[3]);
  }
  p[0] = (p[0] * 0.5f + 0.5f) * static_cast<svtkm::Float32>(Canvas->GetWidth());
  p[1] = (p[1] * 0.5f + 0.5f) * static_cast<svtkm::Float32>(Canvas->GetHeight());
  p[2] = (p[2] * 0.5f + 0.5f) - 0.001f;
  return p;
}
}
} // namespace svtkm::rendering
