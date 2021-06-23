//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_RayTracer_h
#define svtk_m_rendering_raytracing_RayTracer_h

#include <memory>
#include <vector>

#include <svtkm/cont/DataSet.h>

#include <svtkm/rendering/raytracing/Camera.h>
#include <svtkm/rendering/raytracing/TriangleIntersector.h>
namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class SVTKM_RENDERING_EXPORT RayTracer
{
protected:
  std::vector<std::shared_ptr<ShapeIntersector>> Intersectors;
  Camera camera;
  svtkm::cont::Field ScalarField;
  svtkm::cont::ArrayHandle<svtkm::Float32> Scalars;
  svtkm::Id NumberOfShapes;
  svtkm::cont::ArrayHandle<svtkm::Vec4f_32> ColorMap;
  svtkm::Range ScalarRange;
  bool Shade;

  template <typename Precision>
  void RenderOnDevice(Ray<Precision>& rays);

public:
  SVTKM_CONT
  RayTracer();
  SVTKM_CONT
  ~RayTracer();

  SVTKM_CONT
  Camera& GetCamera();

  SVTKM_CONT
  void AddShapeIntersector(std::shared_ptr<ShapeIntersector> intersector);

  SVTKM_CONT
  void SetField(const svtkm::cont::Field& scalarField, const svtkm::Range& scalarRange);

  SVTKM_CONT
  void SetColorMap(const svtkm::cont::ArrayHandle<svtkm::Vec4f_32>& colorMap);

  SVTKM_CONT
  void SetShadingOn(bool on);

  SVTKM_CONT
  void Render(svtkm::rendering::raytracing::Ray<svtkm::Float32>& rays);

  SVTKM_CONT
  void Render(svtkm::rendering::raytracing::Ray<svtkm::Float64>& rays);

  SVTKM_CONT
  svtkm::Id GetNumberOfShapes() const;

  SVTKM_CONT
  void Clear();

}; //class RayTracer
}
}
} // namespace svtkm::rendering::raytracing
#endif //svtk_m_rendering_raytracing_RayTracer_h
