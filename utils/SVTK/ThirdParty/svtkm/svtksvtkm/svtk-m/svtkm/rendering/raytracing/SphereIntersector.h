//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_Sphere_Intersector_h
#define svtk_m_rendering_raytracing_Sphere_Intersector_h

#include <svtkm/rendering/raytracing/ShapeIntersector.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class SphereIntersector : public ShapeIntersector
{
protected:
  svtkm::cont::ArrayHandle<svtkm::Id> PointIds;
  svtkm::cont::ArrayHandle<svtkm::Float32> Radii;

public:
  SphereIntersector();
  virtual ~SphereIntersector() override;

  void SetData(const svtkm::cont::CoordinateSystem& coords,
               svtkm::cont::ArrayHandle<svtkm::Id> pointIds,
               svtkm::cont::ArrayHandle<svtkm::Float32> radii);

  void IntersectRays(Ray<svtkm::Float32>& rays, bool returnCellIndex = false) override;


  void IntersectRays(Ray<svtkm::Float64>& rays, bool returnCellIndex = false) override;

  template <typename Precision>
  void IntersectRaysImp(Ray<Precision>& rays, bool returnCellIndex);


  template <typename Precision>
  void IntersectionDataImp(Ray<Precision>& rays,
                           const svtkm::cont::Field scalarField,
                           const svtkm::Range& scalarRange);

  void IntersectionData(Ray<svtkm::Float32>& rays,
                        const svtkm::cont::Field scalarField,
                        const svtkm::Range& scalarRange) override;

  void IntersectionData(Ray<svtkm::Float64>& rays,
                        const svtkm::cont::Field scalarField,
                        const svtkm::Range& scalarRange) override;

  svtkm::Id GetNumberOfShapes() const override;
}; // class ShapeIntersector
}
}
} //namespace svtkm::rendering::raytracing
#endif //svtk_m_rendering_raytracing_Shape_Intersector_h
