//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <math.h>

#include <svtkm/Math.h>
#include <svtkm/VectorAnalysis.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/DeviceAdapter.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>
#include <svtkm/cont/TryExecute.h>

#include <svtkm/cont/AtomicArray.h>

#include <svtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <svtkm/rendering/raytracing/Logger.h>
#include <svtkm/rendering/raytracing/MortonCodes.h>
#include <svtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <svtkm/rendering/raytracing/Worklets.h>

#include <svtkm/worklet/WorkletMapField.h>

#define AABB_EPSILON 0.00001f
namespace svtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class LinearBVHBuilder
{
public:
  class CountingIterator;

  class GatherFloat32;

  template <typename Device>
  class GatherVecCast;

  class CreateLeafs;

  class BVHData;

  class PropagateAABBs;

  class TreeBuilder;

  SVTKM_CONT
  LinearBVHBuilder() {}

  SVTKM_CONT void SortAABBS(BVHData& bvh, bool);

  SVTKM_CONT void BuildHierarchy(BVHData& bvh);

  SVTKM_CONT void Build(LinearBVH& linearBVH);
}; // class LinearBVHBuilder

class LinearBVHBuilder::CountingIterator : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  CountingIterator() {}
  using ControlSignature = void(FieldOut);
  using ExecutionSignature = void(WorkIndex, _1);
  SVTKM_EXEC
  void operator()(const svtkm::Id& index, svtkm::Id& outId) const { outId = index; }
}; //class countingIterator

class LinearBVHBuilder::GatherFloat32 : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  GatherFloat32() {}
  using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3);

  template <typename InType, typename OutType>
  SVTKM_EXEC void operator()(const svtkm::Id& outIndex,
                            const svtkm::Id& inIndex,
                            const InType& inPortal,
                            OutType& outPortal) const
  {
    outPortal.Set(outIndex, inPortal.Get(inIndex));
  }
}; //class GatherFloat

class LinearBVHBuilder::CreateLeafs : public svtkm::worklet::WorkletMapField
{

public:
  SVTKM_CONT
  CreateLeafs() {}

  typedef void ControlSignature(FieldIn, WholeArrayOut);
  typedef void ExecutionSignature(_1, _2, WorkIndex);

  template <typename LeafPortalType>
  SVTKM_EXEC void operator()(const svtkm::Id& dataIndex,
                            LeafPortalType& leafs,
                            const svtkm::Id& index) const
  {
    const svtkm::Id offset = index * 2;
    leafs.Set(offset, 1);             // number of primitives
    leafs.Set(offset + 1, dataIndex); // number of primitives
  }
}; //class createLeafs

template <typename DeviceAdapterTag>
class LinearBVHBuilder::GatherVecCast : public svtkm::worklet::WorkletMapField
{
private:
  using Vec4IdArrayHandle = typename svtkm::cont::ArrayHandle<svtkm::Id4>;
  using Vec4IntArrayHandle = typename svtkm::cont::ArrayHandle<svtkm::Vec4i_32>;
  using PortalConst = typename Vec4IdArrayHandle::ExecutionTypes<DeviceAdapterTag>::PortalConst;
  using Portal = typename Vec4IntArrayHandle::ExecutionTypes<DeviceAdapterTag>::Portal;

private:
  PortalConst InputPortal;
  Portal OutputPortal;

public:
  SVTKM_CONT
  GatherVecCast(const Vec4IdArrayHandle& inputPortal,
                Vec4IntArrayHandle& outputPortal,
                const svtkm::Id& size)
    : InputPortal(inputPortal.PrepareForInput(DeviceAdapterTag()))
  {
    this->OutputPortal = outputPortal.PrepareForOutput(size, DeviceAdapterTag());
  }
  using ControlSignature = void(FieldIn);
  using ExecutionSignature = void(WorkIndex, _1);
  SVTKM_EXEC
  void operator()(const svtkm::Id& outIndex, const svtkm::Id& inIndex) const
  {
    OutputPortal.Set(outIndex, InputPortal.Get(inIndex));
  }
}; //class GatherVec3Id

class LinearBVHBuilder::BVHData
{
public:
  svtkm::cont::ArrayHandle<svtkm::UInt32> mortonCodes;
  svtkm::cont::ArrayHandle<svtkm::Id> parent;
  svtkm::cont::ArrayHandle<svtkm::Id> leftChild;
  svtkm::cont::ArrayHandle<svtkm::Id> rightChild;
  svtkm::cont::ArrayHandle<svtkm::Id> leafs;
  svtkm::cont::ArrayHandle<svtkm::Bounds> innerBounds;
  svtkm::cont::ArrayHandleCounting<svtkm::Id> leafOffsets;
  AABBs& AABB;

  SVTKM_CONT BVHData(svtkm::Id numPrimitives, AABBs& aabbs)
    : leafOffsets(0, 2, numPrimitives)
    , AABB(aabbs)
    , NumPrimitives(numPrimitives)
  {
    InnerNodeCount = NumPrimitives - 1;
    svtkm::Id size = NumPrimitives + InnerNodeCount;

    parent.Allocate(size);
    leftChild.Allocate(InnerNodeCount);
    rightChild.Allocate(InnerNodeCount);
    innerBounds.Allocate(InnerNodeCount);
    mortonCodes.Allocate(NumPrimitives);
  }

  SVTKM_CONT
  ~BVHData() {}

  SVTKM_CONT
  svtkm::Id GetNumberOfPrimitives() const { return NumPrimitives; }
  SVTKM_CONT
  svtkm::Id GetNumberOfInnerNodes() const { return InnerNodeCount; }

private:
  svtkm::Id NumPrimitives;
  svtkm::Id InnerNodeCount;

}; // class BVH

class LinearBVHBuilder::PropagateAABBs : public svtkm::worklet::WorkletMapField
{
private:
  svtkm::Int32 LeafCount;

public:
  SVTKM_CONT
  PropagateAABBs(svtkm::Int32 leafCount)
    : LeafCount(leafCount)

  {
  }
  using ControlSignature = void(WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,     //Parents
                                WholeArrayIn,     //lchild
                                WholeArrayIn,     //rchild
                                AtomicArrayInOut, //counters
                                WholeArrayInOut   // flatbvh
                                );
  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12);

  template <typename InputPortalType,
            typename OffsetPortalType,
            typename IdPortalType,
            typename AtomicType,
            typename BVHType>
  SVTKM_EXEC_CONT void operator()(const svtkm::Id workIndex,
                                 const InputPortalType& xmin,
                                 const InputPortalType& ymin,
                                 const InputPortalType& zmin,
                                 const InputPortalType& xmax,
                                 const InputPortalType& ymax,
                                 const InputPortalType& zmax,
                                 const OffsetPortalType& leafOffsets,
                                 const IdPortalType& parents,
                                 const IdPortalType& leftChildren,
                                 const IdPortalType& rightChildren,
                                 AtomicType& counters,
                                 BVHType& flatBVH) const

  {
    //move up into the inner nodes
    svtkm::Id currentNode = LeafCount - 1 + workIndex;
    svtkm::Id2 childVector;
    while (currentNode != 0)
    {
      currentNode = parents.Get(currentNode);
      svtkm::Int32 oldCount = counters.Add(currentNode, 1);
      if (oldCount == 0)
      {
        return;
      }
      svtkm::Id currentNodeOffset = currentNode * 4;
      childVector[0] = leftChildren.Get(currentNode);
      childVector[1] = rightChildren.Get(currentNode);
      if (childVector[0] > (LeafCount - 2))
      {
        //our left child is a leaf, so just grab the AABB
        //and set it in the current node
        childVector[0] = childVector[0] - LeafCount + 1;

        svtkm::Vec4f_32 first4Vec; // = FlatBVH.Get(currentNode); only this one needs effects this

        first4Vec[0] = xmin.Get(childVector[0]);
        first4Vec[1] = ymin.Get(childVector[0]);
        first4Vec[2] = zmin.Get(childVector[0]);
        first4Vec[3] = xmax.Get(childVector[0]);
        flatBVH.Set(currentNodeOffset, first4Vec);

        svtkm::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);
        second4Vec[0] = ymax.Get(childVector[0]);
        second4Vec[1] = zmax.Get(childVector[0]);
        flatBVH.Set(currentNodeOffset + 1, second4Vec);
        // set index to leaf
        svtkm::Id leafIndex = leafOffsets.Get(childVector[0]);
        childVector[0] = -(leafIndex + 1);
      }
      else
      {
        //our left child is an inner node, so gather
        //both AABBs in the child and join them for
        //the current node left AABB.
        svtkm::Id child = childVector[0] * 4;

        svtkm::Vec4f_32 cFirst4Vec = flatBVH.Get(child);
        svtkm::Vec4f_32 cSecond4Vec = flatBVH.Get(child + 1);
        svtkm::Vec4f_32 cThird4Vec = flatBVH.Get(child + 2);

        cFirst4Vec[0] = svtkm::Min(cFirst4Vec[0], cSecond4Vec[2]);
        cFirst4Vec[1] = svtkm::Min(cFirst4Vec[1], cSecond4Vec[3]);
        cFirst4Vec[2] = svtkm::Min(cFirst4Vec[2], cThird4Vec[0]);
        cFirst4Vec[3] = svtkm::Max(cFirst4Vec[3], cThird4Vec[1]);
        flatBVH.Set(currentNodeOffset, cFirst4Vec);

        svtkm::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);
        second4Vec[0] = svtkm::Max(cSecond4Vec[0], cThird4Vec[2]);
        second4Vec[1] = svtkm::Max(cSecond4Vec[1], cThird4Vec[3]);

        flatBVH.Set(currentNodeOffset + 1, second4Vec);
      }

      if (childVector[1] > (LeafCount - 2))
      {
        //our right child is a leaf, so just grab the AABB
        //and set it in the current node
        childVector[1] = childVector[1] - LeafCount + 1;


        svtkm::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);

        second4Vec[2] = xmin.Get(childVector[1]);
        second4Vec[3] = ymin.Get(childVector[1]);
        flatBVH.Set(currentNodeOffset + 1, second4Vec);

        svtkm::Vec4f_32 third4Vec;
        third4Vec[0] = zmin.Get(childVector[1]);
        third4Vec[1] = xmax.Get(childVector[1]);
        third4Vec[2] = ymax.Get(childVector[1]);
        third4Vec[3] = zmax.Get(childVector[1]);
        flatBVH.Set(currentNodeOffset + 2, third4Vec);

        // set index to leaf
        svtkm::Id leafIndex = leafOffsets.Get(childVector[1]);
        childVector[1] = -(leafIndex + 1);
      }
      else
      {
        //our left child is an inner node, so gather
        //both AABBs in the child and join them for
        //the current node left AABB.
        svtkm::Id child = childVector[1] * 4;

        svtkm::Vec4f_32 cFirst4Vec = flatBVH.Get(child);
        svtkm::Vec4f_32 cSecond4Vec = flatBVH.Get(child + 1);
        svtkm::Vec4f_32 cThird4Vec = flatBVH.Get(child + 2);

        svtkm::Vec4f_32 second4Vec = flatBVH.Get(currentNodeOffset + 1);
        second4Vec[2] = svtkm::Min(cFirst4Vec[0], cSecond4Vec[2]);
        second4Vec[3] = svtkm::Min(cFirst4Vec[1], cSecond4Vec[3]);
        flatBVH.Set(currentNodeOffset + 1, second4Vec);

        cThird4Vec[0] = svtkm::Min(cFirst4Vec[2], cThird4Vec[0]);
        cThird4Vec[1] = svtkm::Max(cFirst4Vec[3], cThird4Vec[1]);
        cThird4Vec[2] = svtkm::Max(cSecond4Vec[0], cThird4Vec[2]);
        cThird4Vec[3] = svtkm::Max(cSecond4Vec[1], cThird4Vec[3]);
        flatBVH.Set(currentNodeOffset + 2, cThird4Vec);
      }
      svtkm::Vec4f_32 fourth4Vec;
      svtkm::Int32 leftChild =
        static_cast<svtkm::Int32>((childVector[0] >= 0) ? childVector[0] * 4 : childVector[0]);
      memcpy(&fourth4Vec[0], &leftChild, 4);
      svtkm::Int32 rightChild =
        static_cast<svtkm::Int32>((childVector[1] >= 0) ? childVector[1] * 4 : childVector[1]);
      memcpy(&fourth4Vec[1], &rightChild, 4);
      flatBVH.Set(currentNodeOffset + 3, fourth4Vec);
    }
  }
}; //class PropagateAABBs

class LinearBVHBuilder::TreeBuilder : public svtkm::worklet::WorkletMapField
{
private:
  svtkm::Id LeafCount;
  svtkm::Id InnerCount;
  //TODO: get intrinsic support
  SVTKM_EXEC
  inline svtkm::Int32 CountLeadingZeros(svtkm::UInt32& x) const
  {
    svtkm::UInt32 y;
    svtkm::UInt32 n = 32;
    y = x >> 16;
    if (y != 0)
    {
      n = n - 16;
      x = y;
    }
    y = x >> 8;
    if (y != 0)
    {
      n = n - 8;
      x = y;
    }
    y = x >> 4;
    if (y != 0)
    {
      n = n - 4;
      x = y;
    }
    y = x >> 2;
    if (y != 0)
    {
      n = n - 2;
      x = y;
    }
    y = x >> 1;
    if (y != 0)
      return svtkm::Int32(n - 2);
    return svtkm::Int32(n - x);
  }

  // returns the count of largest shared prefix between
  // two morton codes. Ties are broken by the indexes
  // a and b.
  //
  // returns count of the largest binary prefix

  template <typename MortonType>
  SVTKM_EXEC inline svtkm::Int32 delta(const svtkm::Int32& a,
                                     const svtkm::Int32& b,
                                     const MortonType& mortonCodePortal) const
  {
    bool tie = false;
    bool outOfRange = (b < 0 || b > LeafCount - 1);
    //still make the call but with a valid adderss
    svtkm::Int32 bb = (outOfRange) ? 0 : b;
    svtkm::UInt32 aCode = mortonCodePortal.Get(a);
    svtkm::UInt32 bCode = mortonCodePortal.Get(bb);
    //use xor to find where they differ
    svtkm::UInt32 exOr = aCode ^ bCode;
    tie = (exOr == 0);
    //break the tie, a and b must always differ
    exOr = tie ? svtkm::UInt32(a) ^ svtkm::UInt32(bb) : exOr;
    svtkm::Int32 count = CountLeadingZeros(exOr);
    if (tie)
      count += 32;
    count = (outOfRange) ? -1 : count;
    return count;
  }

public:
  SVTKM_CONT
  TreeBuilder(const svtkm::Id& leafCount)
    : LeafCount(leafCount)
    , InnerCount(leafCount - 1)
  {
  }
  using ControlSignature = void(FieldOut, FieldOut, WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(WorkIndex, _1, _2, _3, _4);

  template <typename MortonType, typename ParentType>
  SVTKM_EXEC void operator()(const svtkm::Id& index,
                            svtkm::Id& leftChild,
                            svtkm::Id& rightChild,
                            const MortonType& mortonCodePortal,
                            ParentType& parentPortal) const
  {
    svtkm::Int32 idx = svtkm::Int32(index);
    //determine range direction
    svtkm::Int32 d =
      0 > (delta(idx, idx + 1, mortonCodePortal) - delta(idx, idx - 1, mortonCodePortal)) ? -1 : 1;

    //find upper bound for the length of the range
    svtkm::Int32 minDelta = delta(idx, idx - d, mortonCodePortal);
    svtkm::Int32 lMax = 2;
    while (delta(idx, idx + lMax * d, mortonCodePortal) > minDelta)
      lMax *= 2;

    //binary search to find the lower bound
    svtkm::Int32 l = 0;
    for (int t = lMax / 2; t >= 1; t /= 2)
    {
      if (delta(idx, idx + (l + t) * d, mortonCodePortal) > minDelta)
        l += t;
    }

    svtkm::Int32 j = idx + l * d;
    svtkm::Int32 deltaNode = delta(idx, j, mortonCodePortal);
    svtkm::Int32 s = 0;
    svtkm::Float32 divFactor = 2.f;
    //find the split position using a binary search
    for (svtkm::Int32 t = (svtkm::Int32)ceil(svtkm::Float32(l) / divFactor);;
         divFactor *= 2, t = (svtkm::Int32)ceil(svtkm::Float32(l) / divFactor))
    {
      if (delta(idx, idx + (s + t) * d, mortonCodePortal) > deltaNode)
      {
        s += t;
      }

      if (t == 1)
        break;
    }

    svtkm::Int32 split = idx + s * d + svtkm::Min(d, 0);
    //assign parent/child pointers
    if (svtkm::Min(idx, j) == split)
    {
      //leaf
      parentPortal.Set(split + InnerCount, idx);
      leftChild = split + InnerCount;
    }
    else
    {
      //inner node
      parentPortal.Set(split, idx);
      leftChild = split;
    }


    if (svtkm::Max(idx, j) == split + 1)
    {
      //leaf
      parentPortal.Set(split + InnerCount + 1, idx);
      rightChild = split + InnerCount + 1;
    }
    else
    {
      parentPortal.Set(split + 1, idx);
      rightChild = split + 1;
    }
  }
}; // class TreeBuilder

SVTKM_CONT void LinearBVHBuilder::SortAABBS(BVHData& bvh, bool singleAABB)
{
  //create array of indexes to be sorted with morton codes
  svtkm::cont::ArrayHandle<svtkm::Id> iterator;
  iterator.Allocate(bvh.GetNumberOfPrimitives());

  svtkm::worklet::DispatcherMapField<CountingIterator> iterDispatcher;
  iterDispatcher.Invoke(iterator);

  //sort the morton codes

  svtkm::cont::Algorithm::SortByKey(bvh.mortonCodes, iterator);

  svtkm::Id arraySize = bvh.GetNumberOfPrimitives();
  svtkm::cont::ArrayHandle<svtkm::Float32> temp1;
  svtkm::cont::ArrayHandle<svtkm::Float32> temp2;
  temp1.Allocate(arraySize);

  svtkm::worklet::DispatcherMapField<GatherFloat32> gatherDispatcher;

  //xmins
  gatherDispatcher.Invoke(iterator, bvh.AABB.xmins, temp1);

  temp2 = bvh.AABB.xmins;
  bvh.AABB.xmins = temp1;
  temp1 = temp2;
  //ymins
  gatherDispatcher.Invoke(iterator, bvh.AABB.ymins, temp1);

  temp2 = bvh.AABB.ymins;
  bvh.AABB.ymins = temp1;
  temp1 = temp2;
  //zmins
  gatherDispatcher.Invoke(iterator, bvh.AABB.zmins, temp1);

  temp2 = bvh.AABB.zmins;
  bvh.AABB.zmins = temp1;
  temp1 = temp2;
  //xmaxs
  gatherDispatcher.Invoke(iterator, bvh.AABB.xmaxs, temp1);

  temp2 = bvh.AABB.xmaxs;
  bvh.AABB.xmaxs = temp1;
  temp1 = temp2;
  //ymaxs
  gatherDispatcher.Invoke(iterator, bvh.AABB.ymaxs, temp1);

  temp2 = bvh.AABB.ymaxs;
  bvh.AABB.ymaxs = temp1;
  temp1 = temp2;
  //zmaxs
  gatherDispatcher.Invoke(iterator, bvh.AABB.zmaxs, temp1);

  temp2 = bvh.AABB.zmaxs;
  bvh.AABB.zmaxs = temp1;
  temp1 = temp2;

  // Create the leaf references
  bvh.leafs.Allocate(arraySize * 2);
  // we only actually have a single primitive, but the algorithm
  // requires 2. Make sure they both point to the original
  // primitive
  if (singleAABB)
  {
    auto iterPortal = iterator.GetPortalControl();
    for (int i = 0; i < 2; ++i)
    {
      iterPortal.Set(i, 0);
    }
  }

  svtkm::worklet::DispatcherMapField<CreateLeafs> leafDispatcher;
  leafDispatcher.Invoke(iterator, bvh.leafs);

} // method SortAABB

SVTKM_CONT void LinearBVHBuilder::Build(LinearBVH& linearBVH)
{

  //
  //
  // This algorithm needs at least 2 AABBs
  //
  bool singleAABB = false;
  svtkm::Id numberOfAABBs = linearBVH.GetNumberOfAABBs();
  if (numberOfAABBs == 1)
  {
    numberOfAABBs = 2;
    singleAABB = true;
    svtkm::Float32 xmin = linearBVH.AABB.xmins.GetPortalControl().Get(0);
    svtkm::Float32 ymin = linearBVH.AABB.ymins.GetPortalControl().Get(0);
    svtkm::Float32 zmin = linearBVH.AABB.zmins.GetPortalControl().Get(0);
    svtkm::Float32 xmax = linearBVH.AABB.xmaxs.GetPortalControl().Get(0);
    svtkm::Float32 ymax = linearBVH.AABB.ymaxs.GetPortalControl().Get(0);
    svtkm::Float32 zmax = linearBVH.AABB.zmaxs.GetPortalControl().Get(0);

    linearBVH.AABB.xmins.Allocate(2);
    linearBVH.AABB.ymins.Allocate(2);
    linearBVH.AABB.zmins.Allocate(2);
    linearBVH.AABB.xmaxs.Allocate(2);
    linearBVH.AABB.ymaxs.Allocate(2);
    linearBVH.AABB.zmaxs.Allocate(2);
    for (int i = 0; i < 2; ++i)
    {
      linearBVH.AABB.xmins.GetPortalControl().Set(i, xmin);
      linearBVH.AABB.ymins.GetPortalControl().Set(i, ymin);
      linearBVH.AABB.zmins.GetPortalControl().Set(i, zmin);
      linearBVH.AABB.xmaxs.GetPortalControl().Set(i, xmax);
      linearBVH.AABB.ymaxs.GetPortalControl().Set(i, ymax);
      linearBVH.AABB.zmaxs.GetPortalControl().Set(i, zmax);
    }
  }


  const svtkm::Id numBBoxes = numberOfAABBs;
  BVHData bvh(numBBoxes, linearBVH.GetAABBs());


  // Find the extent of all bounding boxes to generate normalization for morton codes
  svtkm::Vec3f_32 minExtent(svtkm::Infinity32(), svtkm::Infinity32(), svtkm::Infinity32());
  svtkm::Vec3f_32 maxExtent(
    svtkm::NegativeInfinity32(), svtkm::NegativeInfinity32(), svtkm::NegativeInfinity32());
  maxExtent[0] = svtkm::cont::Algorithm::Reduce(bvh.AABB.xmaxs, maxExtent[0], MaxValue());
  maxExtent[1] = svtkm::cont::Algorithm::Reduce(bvh.AABB.ymaxs, maxExtent[1], MaxValue());
  maxExtent[2] = svtkm::cont::Algorithm::Reduce(bvh.AABB.zmaxs, maxExtent[2], MaxValue());
  minExtent[0] = svtkm::cont::Algorithm::Reduce(bvh.AABB.xmins, minExtent[0], MinValue());
  minExtent[1] = svtkm::cont::Algorithm::Reduce(bvh.AABB.ymins, minExtent[1], MinValue());
  minExtent[2] = svtkm::cont::Algorithm::Reduce(bvh.AABB.zmins, minExtent[2], MinValue());

  linearBVH.TotalBounds.X.Min = minExtent[0];
  linearBVH.TotalBounds.X.Max = maxExtent[0];
  linearBVH.TotalBounds.Y.Min = minExtent[1];
  linearBVH.TotalBounds.Y.Max = maxExtent[1];
  linearBVH.TotalBounds.Z.Min = minExtent[2];
  linearBVH.TotalBounds.Z.Max = maxExtent[2];

  svtkm::Vec3f_32 deltaExtent = maxExtent - minExtent;
  svtkm::Vec3f_32 inverseExtent;
  for (int i = 0; i < 3; ++i)
  {
    inverseExtent[i] = (deltaExtent[i] == 0.f) ? 0 : 1.f / deltaExtent[i];
  }

  //Generate the morton codes
  svtkm::worklet::DispatcherMapField<MortonCodeAABB> mortonDispatch(
    MortonCodeAABB(inverseExtent, minExtent));
  mortonDispatch.Invoke(bvh.AABB.xmins,
                        bvh.AABB.ymins,
                        bvh.AABB.zmins,
                        bvh.AABB.xmaxs,
                        bvh.AABB.ymaxs,
                        bvh.AABB.zmaxs,
                        bvh.mortonCodes);
  linearBVH.Allocate(bvh.GetNumberOfPrimitives());

  SortAABBS(bvh, singleAABB);

  svtkm::worklet::DispatcherMapField<TreeBuilder> treeDispatch(
    TreeBuilder(bvh.GetNumberOfPrimitives()));
  treeDispatch.Invoke(bvh.leftChild, bvh.rightChild, bvh.mortonCodes, bvh.parent);

  const svtkm::Int32 primitiveCount = svtkm::Int32(bvh.GetNumberOfPrimitives());

  svtkm::cont::ArrayHandle<svtkm::Int32> counters;
  counters.Allocate(bvh.GetNumberOfPrimitives() - 1);

  svtkm::cont::ArrayHandleConstant<svtkm::Int32> zero(0, bvh.GetNumberOfPrimitives() - 1);
  svtkm::cont::Algorithm::Copy(zero, counters);

  svtkm::worklet::DispatcherMapField<PropagateAABBs> propDispatch(PropagateAABBs{ primitiveCount });

  propDispatch.Invoke(bvh.AABB.xmins,
                      bvh.AABB.ymins,
                      bvh.AABB.zmins,
                      bvh.AABB.xmaxs,
                      bvh.AABB.ymaxs,
                      bvh.AABB.zmaxs,
                      bvh.leafOffsets,
                      bvh.parent,
                      bvh.leftChild,
                      bvh.rightChild,
                      counters,
                      linearBVH.FlatBVH);

  linearBVH.Leafs = bvh.leafs;
}
} //namespace detail

LinearBVH::LinearBVH()
  : IsConstructed(false)
  , CanConstruct(false){};

SVTKM_CONT
LinearBVH::LinearBVH(AABBs& aabbs)
  : AABB(aabbs)
  , IsConstructed(false)
  , CanConstruct(true)
{
}

SVTKM_CONT
LinearBVH::LinearBVH(const LinearBVH& other)
  : AABB(other.AABB)
  , FlatBVH(other.FlatBVH)
  , Leafs(other.Leafs)
  , LeafCount(other.LeafCount)
  , IsConstructed(other.IsConstructed)
  , CanConstruct(other.CanConstruct)
{
}

SVTKM_CONT void LinearBVH::Allocate(const svtkm::Id& leafCount)
{
  LeafCount = leafCount;
  FlatBVH.Allocate((leafCount - 1) * 4);
}

void LinearBVH::Construct()
{
  if (IsConstructed)
    return;
  if (!CanConstruct)
    throw svtkm::cont::ErrorBadValue(
      "Linear BVH: coordinates and triangles must be set before calling construct!");

  detail::LinearBVHBuilder builder;
  builder.Build(*this);
}

SVTKM_CONT
void LinearBVH::SetData(AABBs& aabbs)
{
  AABB = aabbs;
  IsConstructed = false;
  CanConstruct = true;
}

// explicitly export
//template SVTKM_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<
//  svtkm::cont::DeviceAdapterTagSerial>(svtkm::cont::DeviceAdapterTagSerial);
//#ifdef SVTKM_ENABLE_TBB
//template SVTKM_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<svtkm::cont::DeviceAdapterTagTBB>(
//  svtkm::cont::DeviceAdapterTagTBB);
//#endif
//#ifdef SVTKM_ENABLE_OPENMP
//template SVTKM_CONT_EXPORT void LinearBVH::ConstructOnDevice<svtkm::cont::DeviceAdapterTagOpenMP>(
//  svtkm::cont::DeviceAdapterTagOpenMP);
//#endif
//#ifdef SVTKM_ENABLE_CUDA
//template SVTKM_RENDERING_EXPORT void LinearBVH::ConstructOnDevice<svtkm::cont::DeviceAdapterTagCuda>(
//  svtkm::cont::DeviceAdapterTagCuda);
//#endif
//
SVTKM_CONT
bool LinearBVH::GetIsConstructed() const
{
  return IsConstructed;
}

svtkm::Id LinearBVH::GetNumberOfAABBs() const
{
  return AABB.xmins.GetNumberOfValues();
}

AABBs& LinearBVH::GetAABBs()
{
  return AABB;
}
}
}
} // namespace svtkm::rendering::raytracing
