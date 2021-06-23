//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/CellLocatorBoundingIntervalHierarchy.h>

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
#include <svtkm/cont/ErrorBadDevice.h>
#include <svtkm/exec/CellLocatorBoundingIntervalHierarchyExec.h>

#include <svtkm/cont/Invoker.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

namespace svtkm
{
namespace cont
{

using IdArrayHandle = svtkm::cont::ArrayHandle<svtkm::Id>;
using IdPermutationArrayHandle = svtkm::cont::ArrayHandlePermutation<IdArrayHandle, IdArrayHandle>;
using CoordsArrayHandle = svtkm::cont::ArrayHandle<svtkm::FloatDefault>;
using CoordsPermutationArrayHandle =
  svtkm::cont::ArrayHandlePermutation<IdArrayHandle, CoordsArrayHandle>;
using CountingIdArrayHandle = svtkm::cont::ArrayHandleCounting<svtkm::Id>;
using RangeArrayHandle = svtkm::cont::ArrayHandle<svtkm::Range>;
using RangePermutationArrayHandle =
  svtkm::cont::ArrayHandlePermutation<IdArrayHandle, RangeArrayHandle>;
using SplitArrayHandle = svtkm::cont::ArrayHandle<svtkm::worklet::spatialstructure::TreeNode>;
using SplitPermutationArrayHandle =
  svtkm::cont::ArrayHandlePermutation<IdArrayHandle, SplitArrayHandle>;
using SplitPropertiesArrayHandle =
  svtkm::cont::ArrayHandle<svtkm::worklet::spatialstructure::SplitProperties>;

namespace
{

IdArrayHandle CalculateSegmentSizes(const IdArrayHandle& segmentIds, svtkm::Id numCells)
{
  IdArrayHandle discardKeys;
  IdArrayHandle segmentSizes;
  svtkm::cont::Algorithm::ReduceByKey(segmentIds,
                                     svtkm::cont::ArrayHandleConstant<svtkm::Id>(1, numCells),
                                     discardKeys,
                                     segmentSizes,
                                     svtkm::Add());
  return segmentSizes;
}

IdArrayHandle GenerateSegmentIds(const IdArrayHandle& segmentSizes, svtkm::Id numCells)
{
  // Compact segment ids, removing non-contiguous values.

  // 1. Perform ScanInclusive to calculate the end positions of each segment
  IdArrayHandle segmentEnds;
  svtkm::cont::Algorithm::ScanInclusive(segmentSizes, segmentEnds);
  // 2. Perform UpperBounds to perform the final compaction.
  IdArrayHandle segmentIds;
  svtkm::cont::Algorithm::UpperBounds(
    segmentEnds, svtkm::cont::ArrayHandleCounting<svtkm::Id>(0, 1, numCells), segmentIds);
  return segmentIds;
}

void CalculatePlaneSplitCost(svtkm::IdComponent planeIndex,
                             svtkm::IdComponent numPlanes,
                             RangePermutationArrayHandle& segmentRanges,
                             RangeArrayHandle& ranges,
                             CoordsArrayHandle& coords,
                             IdArrayHandle& segmentIds,
                             SplitPropertiesArrayHandle& splits,
                             svtkm::IdComponent index,
                             svtkm::IdComponent numTotalPlanes)
{
  svtkm::cont::Invoker invoker;

  // Make candidate split plane array
  svtkm::cont::ArrayHandle<svtkm::FloatDefault> splitPlanes;
  svtkm::worklet::spatialstructure::SplitPlaneCalculatorWorklet splitPlaneCalcWorklet(planeIndex,
                                                                                     numPlanes);
  invoker(splitPlaneCalcWorklet, segmentRanges, splitPlanes);

  // Check if a point is to the left of the split plane or right
  svtkm::cont::ArrayHandle<svtkm::Id> isLEQOfSplitPlane, isROfSplitPlane;
  invoker(svtkm::worklet::spatialstructure::LEQWorklet{},
          coords,
          splitPlanes,
          isLEQOfSplitPlane,
          isROfSplitPlane);

  // Count of points to the left
  svtkm::cont::ArrayHandle<svtkm::Id> pointsToLeft;
  IdArrayHandle discardKeys;
  svtkm::cont::Algorithm::ReduceByKey(
    segmentIds, isLEQOfSplitPlane, discardKeys, pointsToLeft, svtkm::Add());

  // Count of points to the right
  svtkm::cont::ArrayHandle<svtkm::Id> pointsToRight;
  svtkm::cont::Algorithm::ReduceByKey(
    segmentIds, isROfSplitPlane, discardKeys, pointsToRight, svtkm::Add());

  isLEQOfSplitPlane.ReleaseResourcesExecution();
  isROfSplitPlane.ReleaseResourcesExecution();

  // Calculate Lmax and Rmin
  svtkm::cont::ArrayHandle<svtkm::Range> lMaxRanges;
  {
    svtkm::cont::ArrayHandle<svtkm::Range> leqRanges;
    svtkm::worklet::spatialstructure::FilterRanges<true> worklet;
    invoker(worklet, coords, splitPlanes, ranges, leqRanges);

    svtkm::cont::Algorithm::ReduceByKey(
      segmentIds, leqRanges, discardKeys, lMaxRanges, svtkm::worklet::spatialstructure::RangeAdd());
  }

  svtkm::cont::ArrayHandle<svtkm::Range> rMinRanges;
  {
    svtkm::cont::ArrayHandle<svtkm::Range> rRanges;
    svtkm::worklet::spatialstructure::FilterRanges<false> worklet;
    invoker(worklet, coords, splitPlanes, ranges, rRanges);

    svtkm::cont::Algorithm::ReduceByKey(
      segmentIds, rRanges, discardKeys, rMinRanges, svtkm::worklet::spatialstructure::RangeAdd());
  }

  svtkm::cont::ArrayHandle<svtkm::FloatDefault> segmentedSplitPlanes;
  svtkm::cont::Algorithm::ReduceByKey(
    segmentIds, splitPlanes, discardKeys, segmentedSplitPlanes, svtkm::Minimum());

  // Calculate costs
  svtkm::worklet::spatialstructure::SplitPropertiesCalculator splitPropertiesCalculator(
    index, numTotalPlanes + 1);
  invoker(splitPropertiesCalculator,
          pointsToLeft,
          pointsToRight,
          lMaxRanges,
          rMinRanges,
          segmentedSplitPlanes,
          splits);
}

void CalculateSplitCosts(svtkm::IdComponent numPlanes,
                         RangePermutationArrayHandle& segmentRanges,
                         RangeArrayHandle& ranges,
                         CoordsArrayHandle& coords,
                         IdArrayHandle& segmentIds,
                         SplitPropertiesArrayHandle& splits)
{
  for (svtkm::IdComponent planeIndex = 0; planeIndex < numPlanes; ++planeIndex)
  {
    CalculatePlaneSplitCost(planeIndex,
                            numPlanes,
                            segmentRanges,
                            ranges,
                            coords,
                            segmentIds,
                            splits,
                            planeIndex,
                            numPlanes);
  }
  // Calculate median costs
  CalculatePlaneSplitCost(
    0, 1, segmentRanges, ranges, coords, segmentIds, splits, numPlanes, numPlanes);
}

IdArrayHandle CalculateSplitScatterIndices(const IdArrayHandle& cellIds,
                                           const IdArrayHandle& leqFlags,
                                           const IdArrayHandle& segmentIds)
{
  svtkm::cont::Invoker invoker;

  // Count total number of true flags preceding in segment
  IdArrayHandle trueFlagCounts;
  svtkm::cont::Algorithm::ScanExclusiveByKey(segmentIds, leqFlags, trueFlagCounts);

  // Make a counting iterator.
  CountingIdArrayHandle counts(0, 1, cellIds.GetNumberOfValues());

  // Total number of elements in previous segment
  svtkm::cont::ArrayHandle<svtkm::Id> countPreviousSegments;
  svtkm::cont::Algorithm::ScanInclusiveByKey(
    segmentIds, counts, countPreviousSegments, svtkm::Minimum());

  // Total number of false flags so far in segment
  svtkm::cont::ArrayHandleTransform<IdArrayHandle, svtkm::worklet::spatialstructure::Invert>
    flagsInverse(leqFlags, svtkm::worklet::spatialstructure::Invert());
  svtkm::cont::ArrayHandle<svtkm::Id> runningFalseFlagCount;
  svtkm::cont::Algorithm::ScanInclusiveByKey(
    segmentIds, flagsInverse, runningFalseFlagCount, svtkm::Add());

  // Total number of false flags in segment
  IdArrayHandle totalFalseFlagSegmentCount =
    svtkm::worklet::spatialstructure::ReverseScanInclusiveByKey(
      segmentIds, runningFalseFlagCount, svtkm::Maximum());

  // if point is to the left,
  //    index = total number in  previous segments + total number of false flags in this segment + total number of trues in previous segment
  // else
  //    index = total number in previous segments + number of falses preceding it in the segment.
  IdArrayHandle scatterIndices;
  invoker(svtkm::worklet::spatialstructure::SplitIndicesCalculator{},
          leqFlags,
          trueFlagCounts,
          countPreviousSegments,
          runningFalseFlagCount,
          totalFalseFlagSegmentCount,
          scatterIndices);
  return scatterIndices;
}

} // anonymous namespace

CellLocatorBoundingIntervalHierarchy::~CellLocatorBoundingIntervalHierarchy() = default;


void CellLocatorBoundingIntervalHierarchy::Build()
{
  svtkm::cont::Invoker invoker;

  svtkm::cont::DynamicCellSet cellSet = this->GetCellSet();
  svtkm::Id numCells = cellSet.GetNumberOfCells();
  svtkm::cont::CoordinateSystem coords = this->GetCoordinates();
  svtkm::cont::ArrayHandleVirtualCoordinates points = coords.GetData();

  //std::cout << "No of cells: " << numCells << "\n";
  //std::cout.precision(3);
  //START_TIMER(s11);
  IdArrayHandle cellIds;
  svtkm::cont::Algorithm::Copy(CountingIdArrayHandle(0, 1, numCells), cellIds);
  IdArrayHandle segmentIds;
  svtkm::cont::Algorithm::Copy(svtkm::cont::ArrayHandleConstant<svtkm::Id>(0, numCells), segmentIds);
  //PRINT_TIMER("1.1", s11);

  //START_TIMER(s12);
  CoordsArrayHandle centerXs, centerYs, centerZs;
  RangeArrayHandle xRanges, yRanges, zRanges;
  invoker(svtkm::worklet::spatialstructure::CellRangesExtracter{},
          cellSet,
          points,
          xRanges,
          yRanges,
          zRanges,
          centerXs,
          centerYs,
          centerZs);
  //PRINT_TIMER("1.2", s12);

  bool done = false;
  //svtkm::IdComponent iteration = 0;
  svtkm::Id nodesIndexOffset = 0;
  svtkm::Id numSegments = 1;
  IdArrayHandle discardKeys;
  IdArrayHandle segmentSizes;
  segmentSizes.Allocate(1);
  segmentSizes.GetPortalControl().Set(0, numCells);
  this->ProcessedCellIds.Allocate(numCells);
  svtkm::Id cellIdsOffset = 0;

  IdArrayHandle parentIndices;
  parentIndices.Allocate(1);
  parentIndices.GetPortalControl().Set(0, -1);

  while (!done)
  {
    //std::cout << "**** Iteration " << (++iteration) << " ****\n";
    //Output(segmentSizes);
    //START_TIMER(s21);
    // Calculate the X, Y, Z bounding ranges for each segment
    RangeArrayHandle perSegmentXRanges, perSegmentYRanges, perSegmentZRanges;
    svtkm::cont::Algorithm::ReduceByKey(
      segmentIds, xRanges, discardKeys, perSegmentXRanges, svtkm::Add());
    svtkm::cont::Algorithm::ReduceByKey(
      segmentIds, yRanges, discardKeys, perSegmentYRanges, svtkm::Add());
    svtkm::cont::Algorithm::ReduceByKey(
      segmentIds, zRanges, discardKeys, perSegmentZRanges, svtkm::Add());
    //PRINT_TIMER("2.1", s21);

    // Expand the per segment bounding ranges, to per cell;
    RangePermutationArrayHandle segmentXRanges(segmentIds, perSegmentXRanges);
    RangePermutationArrayHandle segmentYRanges(segmentIds, perSegmentYRanges);
    RangePermutationArrayHandle segmentZRanges(segmentIds, perSegmentZRanges);

    //START_TIMER(s22);
    // Calculate split costs for NumPlanes split planes, across X, Y and Z dimensions
    svtkm::Id numSplitPlanes = numSegments * (this->NumPlanes + 1);
    svtkm::cont::ArrayHandle<svtkm::worklet::spatialstructure::SplitProperties> xSplits, ySplits,
      zSplits;
    xSplits.Allocate(numSplitPlanes);
    ySplits.Allocate(numSplitPlanes);
    zSplits.Allocate(numSplitPlanes);
    CalculateSplitCosts(this->NumPlanes, segmentXRanges, xRanges, centerXs, segmentIds, xSplits);
    CalculateSplitCosts(this->NumPlanes, segmentYRanges, yRanges, centerYs, segmentIds, ySplits);
    CalculateSplitCosts(this->NumPlanes, segmentZRanges, zRanges, centerZs, segmentIds, zSplits);
    //PRINT_TIMER("2.2", s22);

    segmentXRanges.ReleaseResourcesExecution();
    segmentYRanges.ReleaseResourcesExecution();
    segmentZRanges.ReleaseResourcesExecution();

    //START_TIMER(s23);
    // Select best split plane and dimension across X, Y, Z dimension, per segment
    SplitArrayHandle segmentSplits;
    svtkm::cont::ArrayHandle<svtkm::FloatDefault> segmentPlanes;
    svtkm::cont::ArrayHandle<svtkm::Id> splitChoices;
    CountingIdArrayHandle indices(0, 1, numSegments);

    svtkm::worklet::spatialstructure::SplitSelector worklet(
      this->NumPlanes, this->MaxLeafSize, this->NumPlanes + 1);
    invoker(worklet,
            indices,
            xSplits,
            ySplits,
            zSplits,
            segmentSizes,
            segmentSplits,
            segmentPlanes,
            splitChoices);
    //PRINT_TIMER("2.3", s23);

    // Expand the per segment split plane to per cell
    SplitPermutationArrayHandle splits(segmentIds, segmentSplits);
    CoordsPermutationArrayHandle planes(segmentIds, segmentPlanes);

    //START_TIMER(s31);
    IdArrayHandle leqFlags;
    invoker(svtkm::worklet::spatialstructure::CalculateSplitDirectionFlag{},
            centerXs,
            centerYs,
            centerZs,
            splits,
            planes,
            leqFlags);
    //PRINT_TIMER("3.1", s31);

    //START_TIMER(s32);
    IdArrayHandle scatterIndices = CalculateSplitScatterIndices(cellIds, leqFlags, segmentIds);
    IdArrayHandle newSegmentIds;
    IdPermutationArrayHandle sizes(segmentIds, segmentSizes);
    invoker(svtkm::worklet::spatialstructure::SegmentSplitter{ this->MaxLeafSize },
            segmentIds,
            leqFlags,
            sizes,
            newSegmentIds);
    //PRINT_TIMER("3.2", s32);

    //START_TIMER(s33);
    svtkm::cont::ArrayHandle<svtkm::Id> choices;
    svtkm::cont::Algorithm::Copy(IdPermutationArrayHandle(segmentIds, splitChoices), choices);
    cellIds = svtkm::worklet::spatialstructure::ScatterArray(cellIds, scatterIndices);
    segmentIds = svtkm::worklet::spatialstructure::ScatterArray(segmentIds, scatterIndices);
    newSegmentIds = svtkm::worklet::spatialstructure::ScatterArray(newSegmentIds, scatterIndices);
    xRanges = svtkm::worklet::spatialstructure::ScatterArray(xRanges, scatterIndices);
    yRanges = svtkm::worklet::spatialstructure::ScatterArray(yRanges, scatterIndices);
    zRanges = svtkm::worklet::spatialstructure::ScatterArray(zRanges, scatterIndices);
    centerXs = svtkm::worklet::spatialstructure::ScatterArray(centerXs, scatterIndices);
    centerYs = svtkm::worklet::spatialstructure::ScatterArray(centerYs, scatterIndices);
    centerZs = svtkm::worklet::spatialstructure::ScatterArray(centerZs, scatterIndices);
    choices = svtkm::worklet::spatialstructure::ScatterArray(choices, scatterIndices);
    //PRINT_TIMER("3.3", s33);

    // Move the cell ids at leafs to the processed cellids list
    //START_TIMER(s41);
    IdArrayHandle nonSplitSegmentSizes;
    invoker(svtkm::worklet::spatialstructure::NonSplitIndexCalculator{ this->MaxLeafSize },
            segmentSizes,
            nonSplitSegmentSizes);
    IdArrayHandle nonSplitSegmentIndices;
    svtkm::cont::Algorithm::ScanExclusive(nonSplitSegmentSizes, nonSplitSegmentIndices);
    IdArrayHandle runningSplitSegmentCounts;
    svtkm::Id numNewSegments =
      svtkm::cont::Algorithm::ScanExclusive(splitChoices, runningSplitSegmentCounts);
    //PRINT_TIMER("4.1", s41);

    //START_TIMER(s42);
    IdArrayHandle doneCellIds;
    svtkm::cont::Algorithm::CopyIf(
      cellIds, choices, doneCellIds, svtkm::worklet::spatialstructure::Invert());
    svtkm::cont::Algorithm::CopySubRange(
      doneCellIds, 0, doneCellIds.GetNumberOfValues(), this->ProcessedCellIds, cellIdsOffset);

    cellIds = svtkm::worklet::spatialstructure::CopyIfArray(cellIds, choices);
    newSegmentIds = svtkm::worklet::spatialstructure::CopyIfArray(newSegmentIds, choices);
    xRanges = svtkm::worklet::spatialstructure::CopyIfArray(xRanges, choices);
    yRanges = svtkm::worklet::spatialstructure::CopyIfArray(yRanges, choices);
    zRanges = svtkm::worklet::spatialstructure::CopyIfArray(zRanges, choices);
    centerXs = svtkm::worklet::spatialstructure::CopyIfArray(centerXs, choices);
    centerYs = svtkm::worklet::spatialstructure::CopyIfArray(centerYs, choices);
    centerZs = svtkm::worklet::spatialstructure::CopyIfArray(centerZs, choices);
    //PRINT_TIMER("4.2", s42);

    //START_TIMER(s43);
    // Make a new nodes with enough nodes for the current level, copying over the old one
    svtkm::Id nodesSize = this->Nodes.GetNumberOfValues() + numSegments;
    svtkm::cont::ArrayHandle<svtkm::exec::CellLocatorBoundingIntervalHierarchyNode> newTree;
    newTree.Allocate(nodesSize);
    svtkm::cont::Algorithm::CopySubRange(this->Nodes, 0, this->Nodes.GetNumberOfValues(), newTree);

    IdArrayHandle nextParentIndices;
    nextParentIndices.Allocate(2 * numNewSegments);

    CountingIdArrayHandle nodesIndices(nodesIndexOffset, 1, numSegments);
    svtkm::worklet::spatialstructure::TreeLevelAdder nodesAdder(
      cellIdsOffset, nodesSize, this->MaxLeafSize);
    invoker(nodesAdder,
            nodesIndices,
            segmentSplits,
            nonSplitSegmentIndices,
            segmentSizes,
            runningSplitSegmentCounts,
            parentIndices,
            newTree,
            nextParentIndices);
    nodesIndexOffset = nodesSize;
    cellIdsOffset += doneCellIds.GetNumberOfValues();
    this->Nodes = newTree;
    //PRINT_TIMER("4.3", s43);
    //START_TIMER(s51);
    segmentIds = newSegmentIds;
    segmentSizes = CalculateSegmentSizes(segmentIds, segmentIds.GetNumberOfValues());
    segmentIds = GenerateSegmentIds(segmentSizes, segmentIds.GetNumberOfValues());
    IdArrayHandle uniqueSegmentIds;
    svtkm::cont::Algorithm::Copy(segmentIds, uniqueSegmentIds);
    svtkm::cont::Algorithm::Unique(uniqueSegmentIds);
    numSegments = uniqueSegmentIds.GetNumberOfValues();
    done = segmentIds.GetNumberOfValues() == 0;
    parentIndices = nextParentIndices;
    //PRINT_TIMER("5.1", s51);
    //std::cout << "Iteration time: " << iterationTimer.GetElapsedTime() << "\n";
  }
  //std::cout << "Total time: " << totalTimer.GetElapsedTime() << "\n";
}

namespace
{

struct CellLocatorBIHPrepareForExecutionFunctor
{
  template <typename DeviceAdapter, typename CellSetType>
  bool operator()(
    DeviceAdapter,
    const CellSetType& cellset,
    svtkm::cont::VirtualObjectHandle<svtkm::exec::CellLocator>& bihExec,
    const svtkm::cont::ArrayHandle<svtkm::exec::CellLocatorBoundingIntervalHierarchyNode>& nodes,
    const svtkm::cont::ArrayHandle<svtkm::Id>& processedCellIds,
    const svtkm::cont::ArrayHandleVirtualCoordinates& coords) const
  {
    using ExecutionType =
      svtkm::exec::CellLocatorBoundingIntervalHierarchyExec<DeviceAdapter, CellSetType>;
    ExecutionType* execObject =
      new ExecutionType(nodes, processedCellIds, cellset, coords, DeviceAdapter());
    bihExec.Reset(execObject);
    return true;
  }
};

struct BIHCellSetCaster
{
  template <typename CellSet, typename... Args>
  void operator()(CellSet&& cellset, svtkm::cont::DeviceAdapterId device, Args&&... args) const
  {
    //We need to go though CastAndCall first
    const bool success = svtkm::cont::TryExecuteOnDevice(
      device, CellLocatorBIHPrepareForExecutionFunctor(), cellset, std::forward<Args>(args)...);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("BoundingIntervalHierarchy", device);
    }
  }
};
}


const svtkm::exec::CellLocator* CellLocatorBoundingIntervalHierarchy::PrepareForExecution(
  svtkm::cont::DeviceAdapterId device) const
{
  this->GetCellSet().CastAndCall(BIHCellSetCaster{},
                                 device,
                                 this->ExecutionObjectHandle,
                                 this->Nodes,
                                 this->ProcessedCellIds,
                                 this->GetCoordinates().GetData());
  return this->ExecutionObjectHandle.PrepareForExecution(device);
  ;
}

} //namespace cont
} //namespace svtkm
