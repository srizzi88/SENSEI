//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_TriagnleIntersector_h
#define svtk_m_rendering_raytracing_TriagnleIntersector_h

#include <svtkm/cont/DataSet.h>
#include <svtkm/rendering/raytracing/Ray.h>
#include <svtkm/rendering/raytracing/ShapeIntersector.h>
#include <svtkm/rendering/svtkm_rendering_export.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class SVTKM_RENDERING_EXPORT TriangleIntersector : public ShapeIntersector
{
protected:
  svtkm::cont::ArrayHandle<svtkm::Id4> Triangles;
  bool UseWaterTight;

public:
  TriangleIntersector();

  void SetUseWaterTight(bool useIt);

  void SetData(const svtkm::cont::CoordinateSystem& coords,
               svtkm::cont::ArrayHandle<svtkm::Id4> triangles);

  svtkm::cont::ArrayHandle<svtkm::Id4> GetTriangles();
  svtkm::Id GetNumberOfShapes() const override;


  SVTKM_CONT void IntersectRays(Ray<svtkm::Float32>& rays, bool returnCellIndex = false) override;
  SVTKM_CONT void IntersectRays(Ray<svtkm::Float64>& rays, bool returnCellIndex = false) override;


  SVTKM_CONT void IntersectionData(Ray<svtkm::Float32>& rays,
                                  const svtkm::cont::Field scalarField,
                                  const svtkm::Range& scalarRange) override;

  SVTKM_CONT void IntersectionData(Ray<svtkm::Float64>& rays,
                                  const svtkm::cont::Field scalarField,
                                  const svtkm::Range& scalarRange) override;

  template <typename Precision>
  SVTKM_CONT void IntersectRaysImp(Ray<Precision>& rays, bool returnCellIndex);

  template <typename Precision>
  SVTKM_CONT void IntersectionDataImp(Ray<Precision>& rays,
                                     const svtkm::cont::Field scalarField,
                                     const svtkm::Range& scalarRange);

}; // class intersector
}
}
} //namespace svtkm::rendering::raytracing
#endif //svtk_m_rendering_raytracing_TriagnleIntersector_h
