//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_worklet_spatialstructure_BoundingIntervalHierarchy_h
#define svtk_m_worklet_spatialstructure_BoundingIntervalHierarchy_h

#include <type_traits>

#include <svtkm/Bounds.h>
#include <svtkm/Types.h>
#include <svtkm/VecFromPortalPermute.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandlePermutation.h>
#include <svtkm/cont/ArrayHandleReverse.h>
#include <svtkm/cont/ArrayHandleTransform.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/cuda/DeviceAdapterCuda.h>
#include <svtkm/exec/CellLocatorBoundingIntervalHierarchyExec.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

namespace svtkm
{
namespace worklet
{
namespace spatialstructure
{

struct TreeNode
{
  svtkm::FloatDefault LMax;
  svtkm::FloatDefault RMin;
  svtkm::IdComponent Dimension;

  SVTKM_EXEC
  TreeNode()
    : LMax()
    , RMin()
    , Dimension()
  {
  }
}; // struct TreeNode

struct SplitProperties
{
  svtkm::FloatDefault Plane;
  svtkm::Id NumLeftPoints;
  svtkm::Id NumRightPoints;
  svtkm::FloatDefault LMax;
  svtkm::FloatDefault RMin;
  svtkm::FloatDefault Cost;

  SVTKM_EXEC
  SplitProperties()
    : Plane()
    , NumLeftPoints()
    , NumRightPoints()
    , LMax()
    , RMin()
    , Cost()
  {
  }
}; // struct SplitProperties

struct CellRangesExtracter : public svtkm::worklet::WorkletVisitCellsWithPoints
{
  typedef void ControlSignature(CellSetIn,
                                WholeArrayIn,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell,
                                FieldOutCell);
  typedef void ExecutionSignature(_1, PointIndices, _2, _3, _4, _5, _6, _7, _8);

  template <typename CellShape, typename PointIndicesVec, typename PointsPortal>
  SVTKM_EXEC void operator()(CellShape svtkmNotUsed(shape),
                            const PointIndicesVec& pointIndices,
                            const PointsPortal& points,
                            svtkm::Range& rangeX,
                            svtkm::Range& rangeY,
                            svtkm::Range& rangeZ,
                            svtkm::FloatDefault& centerX,
                            svtkm::FloatDefault& centerY,
                            svtkm::FloatDefault& centerZ) const
  {
    svtkm::Bounds bounds;
    svtkm::VecFromPortalPermute<PointIndicesVec, PointsPortal> cellPoints(&pointIndices, points);
    svtkm::IdComponent numPoints = cellPoints.GetNumberOfComponents();
    for (svtkm::IdComponent i = 0; i < numPoints; ++i)
    {
      bounds.Include(cellPoints[i]);
    }
    rangeX = bounds.X;
    rangeY = bounds.Y;
    rangeZ = bounds.Z;
    svtkm::Vec3f center = bounds.Center();
    centerX = center[0];
    centerY = center[1];
    centerZ = center[2];
  }
}; // struct CellRangesExtracter

struct LEQWorklet : public svtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldOut, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  SVTKM_EXEC
  void operator()(const svtkm::FloatDefault& value,
                  const svtkm::FloatDefault& planeValue,
                  svtkm::Id& leq,
                  svtkm::Id& r) const
  {
    leq = value <= planeValue;
    r = !leq;
  }
}; // struct LEQWorklet

template <bool LEQ>
struct FilterRanges;

template <>
struct FilterRanges<true> : public svtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  SVTKM_EXEC
  void operator()(const svtkm::FloatDefault& value,
                  const svtkm::FloatDefault& planeValue,
                  const svtkm::Range& cellBounds,
                  svtkm::Range& outBounds) const
  {
    outBounds = (value <= planeValue) ? cellBounds : svtkm::Range();
  }
}; // struct FilterRanges

template <>
struct FilterRanges<false> : public svtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  SVTKM_EXEC
  void operator()(const svtkm::FloatDefault& value,
                  const svtkm::FloatDefault& planeValue,
                  const svtkm::Range& cellBounds,
                  svtkm::Range& outBounds) const
  {
    outBounds = (value > planeValue) ? cellBounds : svtkm::Range();
  }
}; // struct FilterRanges

struct SplitPlaneCalculatorWorklet : public svtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2);
  using InputDomain = _1;

  SVTKM_CONT
  SplitPlaneCalculatorWorklet(svtkm::IdComponent planeIdx, svtkm::IdComponent numPlanes)
    : Scale(static_cast<svtkm::FloatDefault>(planeIdx + 1) /
            static_cast<svtkm::FloatDefault>(numPlanes + 1))
  {
  }

  SVTKM_EXEC
  void operator()(const svtkm::Range& range, svtkm::FloatDefault& splitPlane) const
  {
    splitPlane = static_cast<svtkm::FloatDefault>(range.Min + Scale * (range.Max - range.Min));
  }

  svtkm::FloatDefault Scale;
};

struct SplitPropertiesCalculator : public svtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, WholeArrayInOut);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, InputIndex);
  using InputDomain = _1;

  SVTKM_CONT
  SplitPropertiesCalculator(svtkm::IdComponent index, svtkm::Id stride)
    : Index(index)
    , Stride(stride)
  {
  }

  template <typename SplitPropertiesPortal>
  SVTKM_EXEC void operator()(const svtkm::Id& pointsToLeft,
                            const svtkm::Id& pointsToRight,
                            const svtkm::Range& lMaxRanges,
                            const svtkm::Range& rMinRanges,
                            const svtkm::FloatDefault& planeValue,
                            SplitPropertiesPortal& splits,
                            svtkm::Id inputIndex) const
  {
    SplitProperties split;
    split.Plane = planeValue;
    split.NumLeftPoints = pointsToLeft;
    split.NumRightPoints = pointsToRight;
    split.LMax = static_cast<svtkm::FloatDefault>(lMaxRanges.Max);
    split.RMin = static_cast<svtkm::FloatDefault>(rMinRanges.Min);
    split.Cost = svtkm::Abs(split.LMax * static_cast<svtkm::FloatDefault>(pointsToLeft) -
                           split.RMin * static_cast<svtkm::FloatDefault>(pointsToRight));
    if (svtkm::IsNan(split.Cost))
    {
      split.Cost = svtkm::Infinity<svtkm::FloatDefault>();
    }
    splits.Set(inputIndex * Stride + Index, split);
    //printf("Plane = %lf, NL = %lld, NR = %lld, LM = %lf, RM = %lf, C = %lf\n", split.Plane, split.NumLeftPoints, split.NumRightPoints, split.LMax, split.RMin, split.Cost);
  }

  svtkm::IdComponent Index;
  svtkm::Id Stride;
};

struct SplitSelector : public svtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  SVTKM_CONT
  SplitSelector(svtkm::IdComponent numPlanes,
                svtkm::IdComponent maxLeafSize,
                svtkm::IdComponent stride)
    : NumPlanes(numPlanes)
    , MaxLeafSize(maxLeafSize)
    , Stride(stride)
  {
  }

  template <typename SplitPropertiesPortal>
  SVTKM_EXEC void operator()(svtkm::Id index,
                            const SplitPropertiesPortal& xSplits,
                            const SplitPropertiesPortal& ySplits,
                            const SplitPropertiesPortal& zSplits,
                            const svtkm::Id& segmentSize,
                            TreeNode& node,
                            svtkm::FloatDefault& plane,
                            svtkm::Id& choice) const
  {
    if (segmentSize <= MaxLeafSize)
    {
      node.Dimension = -1;
      choice = 0;
      return;
    }
    choice = 1;
    using Split = SplitProperties;
    svtkm::FloatDefault minCost = svtkm::Infinity<svtkm::FloatDefault>();
    const Split& xSplit = xSplits[ArgMin(xSplits, index * Stride, Stride)];
    bool found = false;
    if (xSplit.Cost < minCost && xSplit.NumLeftPoints != 0 && xSplit.NumRightPoints != 0)
    {
      minCost = xSplit.Cost;
      node.Dimension = 0;
      node.LMax = xSplit.LMax;
      node.RMin = xSplit.RMin;
      plane = xSplit.Plane;
      found = true;
    }
    const Split& ySplit = ySplits[ArgMin(ySplits, index * Stride, Stride)];
    if (ySplit.Cost < minCost && ySplit.NumLeftPoints != 0 && ySplit.NumRightPoints != 0)
    {
      minCost = ySplit.Cost;
      node.Dimension = 1;
      node.LMax = ySplit.LMax;
      node.RMin = ySplit.RMin;
      plane = ySplit.Plane;
      found = true;
    }
    const Split& zSplit = zSplits[ArgMin(zSplits, index * Stride, Stride)];
    if (zSplit.Cost < minCost && zSplit.NumLeftPoints != 0 && zSplit.NumRightPoints != 0)
    {
      minCost = zSplit.Cost;
      node.Dimension = 2;
      node.LMax = zSplit.LMax;
      node.RMin = zSplit.RMin;
      plane = zSplit.Plane;
      found = true;
    }
    if (!found)
    {
      const Split& xMSplit = xSplits[NumPlanes];
      minCost = xMSplit.Cost;
      node.Dimension = 0;
      node.LMax = xMSplit.LMax;
      node.RMin = xMSplit.RMin;
      plane = xMSplit.Plane;
      const Split& yMSplit = ySplits[NumPlanes];
      if (yMSplit.Cost < minCost && yMSplit.NumLeftPoints != 0 && yMSplit.NumRightPoints != 0)
      {
        minCost = yMSplit.Cost;
        node.Dimension = 1;
        node.LMax = yMSplit.LMax;
        node.RMin = yMSplit.RMin;
        plane = yMSplit.Plane;
      }
      const Split& zMSplit = zSplits[NumPlanes];
      if (zMSplit.Cost < minCost && zMSplit.NumLeftPoints != 0 && zMSplit.NumRightPoints != 0)
      {
        minCost = zMSplit.Cost;
        node.Dimension = 2;
        node.LMax = zMSplit.LMax;
        node.RMin = zMSplit.RMin;
        plane = zMSplit.Plane;
      }
    }
    //printf("Selected plane %lf, with cost %lf [%d, %lf, %lf]\n", plane, minCost, node.Dimension, node.LMax, node.RMin);
  }

  template <typename ArrayPortal>
  SVTKM_EXEC svtkm::Id ArgMin(const ArrayPortal& values, svtkm::Id start, svtkm::Id length) const
  {
    svtkm::Id minIdx = start;
    for (svtkm::Id i = start; i < (start + length); ++i)
    {
      if (values[i].Cost < values[minIdx].Cost)
      {
        minIdx = i;
      }
    }
    return minIdx;
  }

  svtkm::IdComponent NumPlanes;
  svtkm::IdComponent MaxLeafSize;
  svtkm::Id Stride;
};

struct CalculateSplitDirectionFlag : public svtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  SVTKM_EXEC
  void operator()(const svtkm::FloatDefault& x,
                  const svtkm::FloatDefault& y,
                  const svtkm::FloatDefault& z,
                  const TreeNode& split,
                  const svtkm::FloatDefault& plane,
                  svtkm::Id& flag) const
  {
    if (split.Dimension >= 0)
    {
      const svtkm::Vec3f point(x, y, z);
      const svtkm::FloatDefault& c = point[split.Dimension];
      // We use 0 to signify left child, 1 for right child
      flag = 1 - static_cast<svtkm::Id>(c <= plane);
    }
    else
    {
      flag = 0;
    }
  }
}; // struct CalculateSplitDirectionFlag

struct SegmentSplitter : public svtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  using InputDomain = _1;

  SVTKM_CONT
  SegmentSplitter(svtkm::IdComponent maxLeafSize)
    : MaxLeafSize(maxLeafSize)
  {
  }

  SVTKM_EXEC
  void operator()(const svtkm::Id& segmentId,
                  const svtkm::Id& leqFlag,
                  const svtkm::Id& segmentSize,
                  svtkm::Id& newSegmentId) const
  {
    if (segmentSize <= MaxLeafSize)
    {
      // We do not split the segments which have cells fewer than MaxLeafSize, moving them to left
      newSegmentId = 2 * segmentId;
    }
    else
    {
      newSegmentId = 2 * segmentId + leqFlag;
    }
  }

  svtkm::IdComponent MaxLeafSize;
}; // struct SegmentSplitter

struct SplitIndicesCalculator : public svtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  SVTKM_EXEC
  void operator()(const svtkm::Id& leqFlag,
                  const svtkm::Id& trueFlagCount,
                  const svtkm::Id& countPreviousSegment,
                  const svtkm::Id& runningFalseFlagCount,
                  const svtkm::Id& totalFalseFlagCount,
                  svtkm::Id& scatterIndex) const
  {
    if (leqFlag)
    {
      scatterIndex = countPreviousSegment + totalFalseFlagCount + trueFlagCount;
    }
    else
    {
      scatterIndex = countPreviousSegment + runningFalseFlagCount - 1;
    }
  }
}; // struct SplitIndicesCalculator

struct Scatter : public svtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, FieldIn, WholeArrayOut);
  typedef void ExecutionSignature(_1, _2, _3);
  using InputDomain = _1;

  template <typename InputType, typename OutputPortalType>
  SVTKM_EXEC void operator()(const InputType& in, const svtkm::Id& idx, OutputPortalType& out) const
  {
    out.Set(idx, in);
  }
}; // struct Scatter

template <typename ValueArrayHandle, typename IndexArrayHandle>
ValueArrayHandle ScatterArray(const ValueArrayHandle& input, const IndexArrayHandle& indices)
{
  ValueArrayHandle output;
  output.Allocate(input.GetNumberOfValues());
  svtkm::worklet::DispatcherMapField<Scatter>().Invoke(input, indices, output);
  return output;
}

struct NonSplitIndexCalculator : public svtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2);
  using InputDomain = _1;

  SVTKM_CONT
  NonSplitIndexCalculator(svtkm::IdComponent maxLeafSize)
    : MaxLeafSize(maxLeafSize)
  {
  }

  SVTKM_EXEC void operator()(const svtkm::Id& inSegmentSize, svtkm::Id& outSegmentSize) const
  {
    if (inSegmentSize <= MaxLeafSize)
    {
      outSegmentSize = inSegmentSize;
    }
    else
    {
      outSegmentSize = 0;
    }
  }

  svtkm::Id MaxLeafSize;
}; // struct NonSplitIndexCalculator

struct TreeLevelAdder : public svtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn nodeIndices,
                                FieldIn segmentSplits,
                                FieldIn nonSplitSegmentIndices,
                                FieldIn segmentSizes,
                                FieldIn runningSplitSegmentCounts,
                                FieldIn parentIndices,
                                WholeArrayInOut newTree,
                                WholeArrayOut nextParentIndices);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  using InputDomain = _1;

  SVTKM_CONT
  TreeLevelAdder(svtkm::Id cellIdsOffset, svtkm::Id treeOffset, svtkm::IdComponent maxLeafSize)
    : CellIdsOffset(cellIdsOffset)
    , TreeOffset(treeOffset)
    , MaxLeafSize(maxLeafSize)
  {
  }

  template <typename BoundingIntervalHierarchyPortal, typename NextParentPortal>
  SVTKM_EXEC void operator()(svtkm::Id index,
                            const TreeNode& split,
                            svtkm::Id start,
                            svtkm::Id count,
                            svtkm::Id numPreviousSplits,
                            svtkm::Id parentIndex,
                            BoundingIntervalHierarchyPortal& treePortal,
                            NextParentPortal& nextParentPortal) const
  {
    svtkm::exec::CellLocatorBoundingIntervalHierarchyNode node;
    node.ParentIndex = parentIndex;
    if (count > this->MaxLeafSize)
    {
      node.Dimension = split.Dimension;
      node.ChildIndex = this->TreeOffset + 2 * numPreviousSplits;
      node.Node.LMax = split.LMax;
      node.Node.RMin = split.RMin;
      nextParentPortal.Set(2 * numPreviousSplits, index);
      nextParentPortal.Set(2 * numPreviousSplits + 1, index);
    }
    else
    {
      node.ChildIndex = -1;
      node.Leaf.Start = this->CellIdsOffset + start;
      node.Leaf.Size = count;
    }
    treePortal.Set(index, node);
  }

  svtkm::Id CellIdsOffset;
  svtkm::Id TreeOffset;
  svtkm::IdComponent MaxLeafSize;
}; // struct TreeLevelAdder

template <typename T, class BinaryFunctor>
svtkm::cont::ArrayHandle<T> ReverseScanInclusiveByKey(const svtkm::cont::ArrayHandle<T>& keys,
                                                     const svtkm::cont::ArrayHandle<T>& values,
                                                     BinaryFunctor binaryFunctor)
{
  svtkm::cont::ArrayHandle<T> result;
  auto reversedResult = svtkm::cont::make_ArrayHandleReverse(result);

  svtkm::cont::Algorithm::ScanInclusiveByKey(svtkm::cont::make_ArrayHandleReverse(keys),
                                            svtkm::cont::make_ArrayHandleReverse(values),
                                            reversedResult,
                                            binaryFunctor);

  return result;
}

template <typename T, typename U>
svtkm::cont::ArrayHandle<T> CopyIfArray(const svtkm::cont::ArrayHandle<T>& input,
                                       const svtkm::cont::ArrayHandle<U>& stencil)
{
  svtkm::cont::ArrayHandle<T> result;
  svtkm::cont::Algorithm::CopyIf(input, stencil, result);

  return result;
}

SVTKM_CONT
struct Invert
{
  SVTKM_EXEC
  svtkm::Id operator()(const svtkm::Id& value) const { return 1 - value; }
}; // struct Invert

SVTKM_CONT
struct RangeAdd
{
  SVTKM_EXEC
  svtkm::Range operator()(const svtkm::Range& accumulator, const svtkm::Range& value) const
  {
    if (value.IsNonEmpty())
    {
      return accumulator.Union(value);
    }
    else
    {
      return accumulator;
    }
  }
}; // struct RangeAdd

} // namespace spatialstructure
} // namespace worklet
} // namespace svtkm

#endif //svtk_m_worklet_spatialstructure_BoundingIntervalHierarchy_h
