//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_BoundingVolumeHierachy_h
#define svtk_m_worklet_BoundingVolumeHierachy_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/rendering/svtkm_rendering_export.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{

struct AABBs
{
  svtkm::cont::ArrayHandle<svtkm::Float32> xmins;
  svtkm::cont::ArrayHandle<svtkm::Float32> ymins;
  svtkm::cont::ArrayHandle<svtkm::Float32> zmins;
  svtkm::cont::ArrayHandle<svtkm::Float32> xmaxs;
  svtkm::cont::ArrayHandle<svtkm::Float32> ymaxs;
  svtkm::cont::ArrayHandle<svtkm::Float32> zmaxs;
};

//
// This is the data structure that is passed to the ray tracer.
//
class SVTKM_RENDERING_EXPORT LinearBVH
{
public:
  using InnerNodesHandle = svtkm::cont::ArrayHandle<svtkm::Vec4f_32>;
  using LeafNodesHandle = svtkm::cont::ArrayHandle<Id>;
  AABBs AABB;
  InnerNodesHandle FlatBVH;
  LeafNodesHandle Leafs;
  svtkm::Bounds TotalBounds;
  svtkm::Id LeafCount;

protected:
  bool IsConstructed;
  bool CanConstruct;

public:
  LinearBVH();

  SVTKM_CONT
  LinearBVH(AABBs& aabbs);

  SVTKM_CONT
  LinearBVH(const LinearBVH& other);

  SVTKM_CONT void Allocate(const svtkm::Id& leafCount);

  SVTKM_CONT
  void Construct();

  SVTKM_CONT
  void SetData(AABBs& aabbs);

  SVTKM_CONT
  AABBs& GetAABBs();

  SVTKM_CONT
  bool GetIsConstructed() const;

  svtkm::Id GetNumberOfAABBs() const;
}; // class LinearBVH
}
}
} // namespace svtkm::rendering::raytracing
#endif //svtk_m_worklet_BoundingVolumeHierachy_h
