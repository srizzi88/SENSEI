//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_CanvasRayTracer_h
#define svtk_m_rendering_CanvasRayTracer_h

#include <svtkm/rendering/svtkm_rendering_export.h>

#include <svtkm/rendering/Canvas.h>
#include <svtkm/rendering/raytracing/Ray.h>

namespace svtkm
{
namespace rendering
{

class SVTKM_RENDERING_EXPORT CanvasRayTracer : public Canvas
{
public:
  CanvasRayTracer(svtkm::Id width = 1024, svtkm::Id height = 1024);

  ~CanvasRayTracer();

  svtkm::rendering::Canvas* NewCopy() const override;

  void WriteToCanvas(const svtkm::rendering::raytracing::Ray<svtkm::Float32>& rays,
                     const svtkm::cont::ArrayHandle<svtkm::Float32>& colors,
                     const svtkm::rendering::Camera& camera);

  void WriteToCanvas(const svtkm::rendering::raytracing::Ray<svtkm::Float64>& rays,
                     const svtkm::cont::ArrayHandle<svtkm::Float64>& colors,
                     const svtkm::rendering::Camera& camera);
}; // class CanvasRayTracer
}
} // namespace svtkm::rendering

#endif //svtk_m_rendering_CanvasRayTracer_h
