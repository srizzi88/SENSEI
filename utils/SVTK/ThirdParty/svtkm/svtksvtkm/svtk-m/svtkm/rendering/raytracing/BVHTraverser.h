//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_rendering_raytracing_BVH_Traverser_h
#define svtk_m_rendering_raytracing_BVH_Traverser_h

#include <svtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <svtkm/rendering/raytracing/Ray.h>
#include <svtkm/rendering/raytracing/RayTracingTypeDefs.h>

#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>

namespace svtkm
{
namespace rendering
{
namespace raytracing
{
#define END_FLAG -1000000000

template <typename BVHPortalType, typename Precision>
SVTKM_EXEC inline bool IntersectAABB(const BVHPortalType& bvh,
                                    const svtkm::Int32& currentNode,
                                    const svtkm::Vec<Precision, 3>& originDir,
                                    const svtkm::Vec<Precision, 3>& invDir,
                                    const Precision& closestDistance,
                                    bool& hitLeftChild,
                                    bool& hitRightChild,
                                    const Precision& minDistance) //Find hit after this distance
{
  svtkm::Vec4f_32 first4 = bvh.Get(currentNode);
  svtkm::Vec4f_32 second4 = bvh.Get(currentNode + 1);
  svtkm::Vec4f_32 third4 = bvh.Get(currentNode + 2);

  Precision xmin0 = first4[0] * invDir[0] - originDir[0];
  Precision ymin0 = first4[1] * invDir[1] - originDir[1];
  Precision zmin0 = first4[2] * invDir[2] - originDir[2];
  Precision xmax0 = first4[3] * invDir[0] - originDir[0];
  Precision ymax0 = second4[0] * invDir[1] - originDir[1];
  Precision zmax0 = second4[1] * invDir[2] - originDir[2];

  Precision min0 = svtkm::Max(
    svtkm::Max(svtkm::Max(svtkm::Min(ymin0, ymax0), svtkm::Min(xmin0, xmax0)), svtkm::Min(zmin0, zmax0)),
    minDistance);
  Precision max0 = svtkm::Min(
    svtkm::Min(svtkm::Min(svtkm::Max(ymin0, ymax0), svtkm::Max(xmin0, xmax0)), svtkm::Max(zmin0, zmax0)),
    closestDistance);
  hitLeftChild = (max0 >= min0);

  Precision xmin1 = second4[2] * invDir[0] - originDir[0];
  Precision ymin1 = second4[3] * invDir[1] - originDir[1];
  Precision zmin1 = third4[0] * invDir[2] - originDir[2];
  Precision xmax1 = third4[1] * invDir[0] - originDir[0];
  Precision ymax1 = third4[2] * invDir[1] - originDir[1];
  Precision zmax1 = third4[3] * invDir[2] - originDir[2];

  Precision min1 = svtkm::Max(
    svtkm::Max(svtkm::Max(svtkm::Min(ymin1, ymax1), svtkm::Min(xmin1, xmax1)), svtkm::Min(zmin1, zmax1)),
    minDistance);
  Precision max1 = svtkm::Min(
    svtkm::Min(svtkm::Min(svtkm::Max(ymin1, ymax1), svtkm::Max(xmin1, xmax1)), svtkm::Max(zmin1, zmax1)),
    closestDistance);
  hitRightChild = (max1 >= min1);
  return (min0 > min1);
}

class BVHTraverser
{
public:
  class Intersector : public svtkm::worklet::WorkletMapField
  {
  private:
    SVTKM_EXEC
    inline svtkm::Float32 rcp(svtkm::Float32 f) const { return 1.0f / f; }
    SVTKM_EXEC
    inline svtkm::Float32 rcp_safe(svtkm::Float32 f) const
    {
      return rcp((svtkm::Abs(f) < 1e-8f) ? 1e-8f : f);
    }
    SVTKM_EXEC
    inline svtkm::Float64 rcp(svtkm::Float64 f) const { return 1.0 / f; }
    SVTKM_EXEC
    inline svtkm::Float64 rcp_safe(svtkm::Float64 f) const
    {
      return rcp((svtkm::Abs(f) < 1e-8f) ? 1e-8f : f);
    }

  public:
    SVTKM_CONT
    Intersector() {}
    using ControlSignature = void(FieldIn,
                                  FieldIn,
                                  FieldOut,
                                  FieldIn,
                                  FieldIn,
                                  FieldOut,
                                  FieldOut,
                                  FieldOut,
                                  WholeArrayIn,
                                  ExecObject leafIntersector,
                                  WholeArrayIn,
                                  WholeArrayIn);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12);


    template <typename PointPortalType,
              typename Precision,
              typename LeafType,
              typename InnerNodePortalType,
              typename LeafPortalType>
    SVTKM_EXEC void operator()(const svtkm::Vec<Precision, 3>& dir,
                              const svtkm::Vec<Precision, 3>& origin,
                              Precision& distance,
                              const Precision& minDistance,
                              const Precision& maxDistance,
                              Precision& minU,
                              Precision& minV,
                              svtkm::Id& hitIndex,
                              const PointPortalType& points,
                              LeafType& leafIntersector,
                              const InnerNodePortalType& flatBVH,
                              const LeafPortalType& leafs) const
    {
      Precision closestDistance = maxDistance;
      distance = maxDistance;
      hitIndex = -1;

      svtkm::Vec<Precision, 3> invDir;
      invDir[0] = rcp_safe(dir[0]);
      invDir[1] = rcp_safe(dir[1]);
      invDir[2] = rcp_safe(dir[2]);
      svtkm::Int32 currentNode;

      svtkm::Int32 todo[64];
      svtkm::Int32 stackptr = 0;
      svtkm::Int32 barrier = (svtkm::Int32)END_FLAG;
      currentNode = 0;

      todo[stackptr] = barrier;

      svtkm::Vec<Precision, 3> originDir = origin * invDir;

      while (currentNode != END_FLAG)
      {
        if (currentNode > -1)
        {


          bool hitLeftChild, hitRightChild;
          bool rightCloser = IntersectAABB(flatBVH,
                                           currentNode,
                                           originDir,
                                           invDir,
                                           closestDistance,
                                           hitLeftChild,
                                           hitRightChild,
                                           minDistance);

          if (!hitLeftChild && !hitRightChild)
          {
            currentNode = todo[stackptr];
            stackptr--;
          }
          else
          {
            svtkm::Vec4f_32 children = flatBVH.Get(currentNode + 3); //Children.Get(currentNode);
            svtkm::Int32 leftChild;
            memcpy(&leftChild, &children[0], 4);
            svtkm::Int32 rightChild;
            memcpy(&rightChild, &children[1], 4);
            currentNode = (hitLeftChild) ? leftChild : rightChild;
            if (hitLeftChild && hitRightChild)
            {
              if (rightCloser)
              {
                currentNode = rightChild;
                stackptr++;
                todo[stackptr] = leftChild;
              }
              else
              {
                stackptr++;
                todo[stackptr] = rightChild;
              }
            }
          }
        } // if inner node

        if (currentNode < 0 && currentNode != barrier) //check register usage
        {
          currentNode = -currentNode - 1; //swap the neg address
          leafIntersector.IntersectLeaf(currentNode,
                                        origin,
                                        dir,
                                        points,
                                        hitIndex,
                                        closestDistance,
                                        minU,
                                        minV,
                                        leafs,
                                        minDistance);
          currentNode = todo[stackptr];
          stackptr--;
        } // if leaf node

      } //while

      if (hitIndex != -1)
        distance = closestDistance;
    } // ()
  };


  template <typename Precision, typename LeafIntersectorType>
  SVTKM_CONT void IntersectRays(Ray<Precision>& rays,
                               LinearBVH& bvh,
                               LeafIntersectorType& leafIntersector,
                               svtkm::cont::CoordinateSystem& coordsHandle)
  {
    svtkm::worklet::DispatcherMapField<Intersector> intersectDispatch;
    intersectDispatch.Invoke(rays.Dir,
                             rays.Origin,
                             rays.Distance,
                             rays.MinDistance,
                             rays.MaxDistance,
                             rays.U,
                             rays.V,
                             rays.HitIdx,
                             coordsHandle,
                             leafIntersector,
                             bvh.FlatBVH,
                             bvh.Leafs);
  }
}; // BVHTraverser
#undef END_FLAG
}
}
} //namespace svtkm::rendering::raytracing
#endif //svtk_m_rendering_raytracing_BVHTraverser_h
