//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_Shape_Intersector_h
#define svtk_m_rendering_raytracing_Shape_Intersector_h

#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DynamicCellSet.h>
#include <svtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <svtkm/rendering/raytracing/Ray.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

class SVTKM_RENDERING_EXPORT ShapeIntersector
{
protected:
  LinearBVH BVH;
  svtkm::cont::CoordinateSystem CoordsHandle;
  svtkm::Bounds ShapeBounds;
  void SetAABBs(AABBs& aabbs);

public:
  ShapeIntersector();
  virtual ~ShapeIntersector();

  //
  //  Intersect Rays finds the nearest intersection shape contained in the derived
  //  class in between min and max distances. HitIdx will be set to the local
  //  primitive id unless returnCellIndex is set to true. Cells are often
  //  decomposed into triangles and setting returnCellIndex to true will set
  //  HitIdx to the id of the cell.
  //
  virtual void IntersectRays(Ray<svtkm::Float32>& rays, bool returnCellIndex = false) = 0;


  virtual void IntersectRays(Ray<svtkm::Float64>& rays, bool returnCellIndex = false) = 0;

  //
  // Calling intersection data directly after IntersectRays popoulates
  // ray data: intersection point, surface normal, and interpolated scalar
  // value at the intersection location. Additionally, distance to intersection
  // becomes the new max distance.
  //
  virtual void IntersectionData(Ray<svtkm::Float32>& rays,
                                const svtkm::cont::Field scalarField,
                                const svtkm::Range& scalarRange) = 0;

  virtual void IntersectionData(Ray<svtkm::Float64>& rays,
                                const svtkm::cont::Field scalarField,
                                const svtkm::Range& scalarRange) = 0;


  template <typename Precision>
  void IntersectionPointImp(Ray<Precision>& rays);
  void IntersectionPoint(Ray<svtkm::Float32>& rays);
  void IntersectionPoint(Ray<svtkm::Float64>& rays);

  svtkm::Bounds GetShapeBounds() const;
  virtual svtkm::Id GetNumberOfShapes() const = 0;
}; // class ShapeIntersector
}
}
} //namespace svtkm::rendering::raytracing
#endif //svtk_m_rendering_raytracing_Shape_Intersector_h
