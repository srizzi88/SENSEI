//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/CellLocatorUniformBins.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleTransform.h>

#include <svtkm/cont/Invoker.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

using namespace svtkm::internal::cl_uniform_bins;

namespace
{

struct BinsBBox
{
  DimVec3 Min;
  DimVec3 Max;

  SVTKM_EXEC
  bool Empty() const
  {
    return (this->Max[0] < this->Min[0]) || (this->Max[1] < this->Min[1]) ||
      (this->Max[2] < this->Min[2]);
  }
};

SVTKM_EXEC_CONT static DimVec3 ComputeGridDimension(svtkm::Id numberOfCells,
                                                   const FloatVec3& size,
                                                   svtkm::FloatDefault density)
{
  svtkm::FloatDefault nsides = 0.0f;
  svtkm::FloatDefault volume = 1.0f;
  svtkm::FloatDefault maxside = svtkm::Max(size[0], svtkm::Max(size[1], size[2]));
  for (int i = 0; i < 3; ++i)
  {
    if (size[i] / maxside >= 1e-4f)
    {
      nsides += 1.0f;
      volume *= size[i];
    }
  }

  auto r =
    svtkm::Pow((static_cast<svtkm::FloatDefault>(numberOfCells) / (volume * density)), 1.0f / nsides);
  return svtkm::Max(DimVec3(1),
                   DimVec3(static_cast<DimensionType>(size[0] * r),
                           static_cast<DimensionType>(size[1] * r),
                           static_cast<DimensionType>(size[2] * r)));
}

SVTKM_EXEC static BinsBBox ComputeIntersectingBins(const Bounds cellBounds, const Grid& grid)
{
  auto minb = static_cast<DimVec3>((cellBounds.Min - grid.Origin) / grid.BinSize);
  auto maxb = static_cast<DimVec3>((cellBounds.Max - grid.Origin) / grid.BinSize);

  return { svtkm::Max(DimVec3(0), minb), svtkm::Min(grid.Dimensions - DimVec3(1), maxb) };
}

SVTKM_EXEC static svtkm::Id GetNumberOfBins(const BinsBBox& binsBBox)
{
  return binsBBox.Empty() ? 0 : ((binsBBox.Max[0] - binsBBox.Min[0] + 1) *
                                 (binsBBox.Max[1] - binsBBox.Min[1] + 1) *
                                 (binsBBox.Max[2] - binsBBox.Min[2] + 1));
}

class BBoxIterator
{
public:
  SVTKM_EXEC_CONT BBoxIterator(const BinsBBox& bbox, const DimVec3& dim)
    : BBox(bbox)
    , Dim(dim)
    , StepY(dim[0] - (bbox.Max[0] - bbox.Min[0] + 1))
    , StepZ((dim[0] * dim[1]) - ((bbox.Max[1] - bbox.Min[1] + 1) * dim[0]))
  {
    this->Init();
  }

  SVTKM_EXEC_CONT void Init()
  {
    this->Idx = this->BBox.Min;
    this->FlatIdx = ComputeFlatIndex(this->Idx, this->Dim);
    this->DoneFlag = this->BBox.Empty();
  }

  SVTKM_EXEC_CONT bool Done() const { return this->DoneFlag; }

  SVTKM_EXEC_CONT void Next()
  {
    if (!this->DoneFlag)
    {
      ++this->Idx[0];
      this->FlatIdx += 1;
      if (this->Idx[0] > this->BBox.Max[0])
      {
        this->Idx[0] = this->BBox.Min[0];
        ++this->Idx[1];
        this->FlatIdx += this->StepY;
        if (this->Idx[1] > this->BBox.Max[1])
        {
          this->Idx[1] = this->BBox.Min[1];
          ++this->Idx[2];
          this->FlatIdx += this->StepZ;
          if (this->Idx[2] > this->BBox.Max[2])
          {
            this->DoneFlag = true;
          }
        }
      }
    }
  }

  SVTKM_EXEC_CONT const DimVec3& GetIdx() const { return this->Idx; }

  SVTKM_EXEC_CONT svtkm::Id GetFlatIdx() const { return this->FlatIdx; }

private:
  const BinsBBox BBox;
  const DimVec3 Dim;
  const svtkm::Id StepY, StepZ;

  DimVec3 Idx;
  svtkm::Id FlatIdx;
  bool DoneFlag;
};

class CountBinsL1 : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, FieldInPoint coords, FieldOutCell bincount);
  using ExecutionSignature = void(_2, _3);

  CountBinsL1(const Grid& grid)
    : L1Grid(grid)
  {
  }

  template <typename PointsVecType>
  SVTKM_EXEC void operator()(const PointsVecType& points, svtkm::Id& numBins) const
  {
    auto cellBounds = ComputeCellBounds(points);
    auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);
    numBins = GetNumberOfBins(binsBBox);
  }

private:
  Grid L1Grid;
};

class FindBinsL1 : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint coords,
                                FieldInCell offsets,
                                WholeArrayOut binIds);
  using ExecutionSignature = void(_2, _3, _4);

  FindBinsL1(const Grid& grid)
    : L1Grid(grid)
  {
  }

  template <typename PointsVecType, typename BinIdsPortalType>
  SVTKM_EXEC void operator()(const PointsVecType& points,
                            svtkm::Id offset,
                            BinIdsPortalType& binIds) const
  {
    auto cellBounds = ComputeCellBounds(points);
    auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);

    for (BBoxIterator i(binsBBox, this->L1Grid.Dimensions); !i.Done(); i.Next())
    {
      binIds.Set(offset, i.GetFlatIdx());
      ++offset;
    }
  }

private:
  Grid L1Grid;
};

class GenerateBinsL1 : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn binIds, FieldIn cellCounts, WholeArrayOut dimensions);
  using ExecutionSignature = void(_1, _2, _3);

  using InputDomain = _1;

  GenerateBinsL1(FloatVec3 size, svtkm::FloatDefault density)
    : Size(size)
    , Density(density)
  {
  }

  template <typename OutputDimensionsPortal>
  SVTKM_EXEC void operator()(svtkm::Id binId,
                            svtkm::Id numCells,
                            OutputDimensionsPortal& dimensions) const
  {
    dimensions.Set(binId, ComputeGridDimension(numCells, this->Size, this->Density));
  }

private:
  FloatVec3 Size;
  svtkm::FloatDefault Density;
};

class CountBinsL2 : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint coords,
                                WholeArrayIn binDimensions,
                                FieldOutCell bincount);
  using ExecutionSignature = void(_2, _3, _4);

  CountBinsL2(const Grid& grid)
    : L1Grid(grid)
  {
  }

  template <typename PointsVecType, typename BinDimensionsPortalType>
  SVTKM_EXEC void operator()(const PointsVecType& points,
                            const BinDimensionsPortalType& binDimensions,
                            svtkm::Id& numBins) const
  {
    auto cellBounds = ComputeCellBounds(points);
    auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);

    numBins = 0;
    for (BBoxIterator i(binsBBox, this->L1Grid.Dimensions); !i.Done(); i.Next())
    {
      Grid leaf = ComputeLeafGrid(i.GetIdx(), binDimensions.Get(i.GetFlatIdx()), this->L1Grid);
      auto binsBBoxL2 = ComputeIntersectingBins(cellBounds, leaf);
      numBins += GetNumberOfBins(binsBBoxL2);
    }
  }

private:
  Grid L1Grid;
};

class FindBinsL2 : public svtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint coords,
                                WholeArrayIn binDimensions,
                                WholeArrayIn binStarts,
                                FieldInCell offsets,
                                WholeArrayOut binIds,
                                WholeArrayOut cellIds);
  using ExecutionSignature = void(InputIndex, _2, _3, _4, _5, _6, _7);

  FindBinsL2(const Grid& grid)
    : L1Grid(grid)
  {
  }

  template <typename PointsVecType,
            typename BinDimensionsPortalType,
            typename BinStartsPortalType,
            typename BinIdsPortalType,
            typename CellIdsPortalType>
  SVTKM_EXEC void operator()(svtkm::Id cellId,
                            const PointsVecType& points,
                            const BinDimensionsPortalType& binDimensions,
                            const BinStartsPortalType& binStarts,
                            svtkm::Id offset,
                            BinIdsPortalType binIds,
                            CellIdsPortalType cellIds) const
  {
    auto cellBounds = ComputeCellBounds(points);
    auto binsBBox = ComputeIntersectingBins(cellBounds, this->L1Grid);

    for (BBoxIterator i(binsBBox, this->L1Grid.Dimensions); !i.Done(); i.Next())
    {
      Grid leaf = ComputeLeafGrid(i.GetIdx(), binDimensions.Get(i.GetFlatIdx()), this->L1Grid);
      auto binsBBoxL2 = ComputeIntersectingBins(cellBounds, leaf);
      svtkm::Id leafStart = binStarts.Get(i.GetFlatIdx());

      for (BBoxIterator j(binsBBoxL2, leaf.Dimensions); !j.Done(); j.Next())
      {
        binIds.Set(offset, leafStart + j.GetFlatIdx());
        cellIds.Set(offset, cellId);
        ++offset;
      }
    }
  }

private:
  Grid L1Grid;
};

class GenerateBinsL2 : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn binIds,
                                FieldIn startsIn,
                                FieldIn countsIn,
                                WholeArrayOut startsOut,
                                WholeArrayOut countsOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  using InputDomain = _1;

  template <typename CellStartsPortalType, typename CellCountsPortalType>
  SVTKM_EXEC void operator()(svtkm::Id binIndex,
                            svtkm::Id start,
                            svtkm::Id count,
                            CellStartsPortalType& cellStarts,
                            CellCountsPortalType& cellCounts) const
  {
    cellStarts.Set(binIndex, start);
    cellCounts.Set(binIndex, count);
  }
};

struct DimensionsToCount
{
  SVTKM_EXEC svtkm::Id operator()(const DimVec3& dim) const { return dim[0] * dim[1] * dim[2]; }
};

} // anonymous namespace

namespace svtkm
{
namespace cont
{

//----------------------------------------------------------------------------
/// Builds the cell locator lookup structure
///
SVTKM_CONT void CellLocatorUniformBins::Build()
{
  svtkm::cont::Invoker invoke;

  auto cellset = this->GetCellSet();
  const auto& coords = this->GetCoordinates();

  // 1: Compute the top level grid
  auto bounds = coords.GetBounds();
  FloatVec3 bmin(static_cast<svtkm::FloatDefault>(bounds.X.Min),
                 static_cast<svtkm::FloatDefault>(bounds.Y.Min),
                 static_cast<svtkm::FloatDefault>(bounds.Z.Min));
  FloatVec3 bmax(static_cast<svtkm::FloatDefault>(bounds.X.Max),
                 static_cast<svtkm::FloatDefault>(bounds.Y.Max),
                 static_cast<svtkm::FloatDefault>(bounds.Z.Max));
  auto size = bmax - bmin;
  auto fudge = svtkm::Max(FloatVec3(1e-6f), size * 1e-4f);
  size += 2.0f * fudge;

  this->TopLevel.Dimensions =
    ComputeGridDimension(cellset.GetNumberOfCells(), size, this->DensityL1);
  this->TopLevel.Origin = bmin - fudge;
  this->TopLevel.BinSize = size / static_cast<FloatVec3>(this->TopLevel.Dimensions);

  // 2: For each cell, find the number of top level bins they intersect
  svtkm::cont::ArrayHandle<svtkm::Id> binCounts;
  CountBinsL1 countL1(this->TopLevel);
  invoke(countL1, cellset, coords, binCounts);

  // 3: Total number of unique (cell, bin) pairs (for pre-allocating arrays)
  svtkm::Id numPairsL1 = svtkm::cont::Algorithm::ScanExclusive(binCounts, binCounts);

  // 4: For each cell find the top level bins that intersect it
  svtkm::cont::ArrayHandle<svtkm::Id> binIds;
  binIds.Allocate(numPairsL1);
  FindBinsL1 findL1(this->TopLevel);
  invoke(findL1, cellset, coords, binCounts, binIds);
  binCounts.ReleaseResources();

  // 5: From above, find the number of cells that intersect each top level bin
  svtkm::cont::Algorithm::Sort(binIds);
  svtkm::cont::ArrayHandle<svtkm::Id> bins;
  svtkm::cont::ArrayHandle<svtkm::Id> cellsPerBin;
  svtkm::cont::Algorithm::ReduceByKey(binIds,
                                     svtkm::cont::make_ArrayHandleConstant(svtkm::Id(1), numPairsL1),
                                     bins,
                                     cellsPerBin,
                                     svtkm::Sum());
  binIds.ReleaseResources();

  // 6: Compute level-2 dimensions
  svtkm::Id numberOfBins =
    this->TopLevel.Dimensions[0] * this->TopLevel.Dimensions[1] * this->TopLevel.Dimensions[2];
  svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandleConstant(DimVec3(0), numberOfBins),
                        this->LeafDimensions);
  GenerateBinsL1 generateL1(this->TopLevel.BinSize, this->DensityL2);
  invoke(generateL1, bins, cellsPerBin, this->LeafDimensions);
  bins.ReleaseResources();
  cellsPerBin.ReleaseResources();

  // 7: Compute number of level-2 bins
  svtkm::Id numberOfLeaves = svtkm::cont::Algorithm::ScanExclusive(
    svtkm::cont::make_ArrayHandleTransform(this->LeafDimensions, DimensionsToCount()),
    this->LeafStartIndex);


  // 8: For each cell, find the number of l2 bins they intersect
  CountBinsL2 countL2(this->TopLevel);
  invoke(countL2, cellset, coords, this->LeafDimensions, binCounts);

  // 9: Total number of unique (cell, bin) pairs (for pre-allocating arrays)
  svtkm::Id numPairsL2 = svtkm::cont::Algorithm::ScanExclusive(binCounts, binCounts);

  // 10: For each cell, find the l2 bins they intersect
  binIds.Allocate(numPairsL2);
  this->CellIds.Allocate(numPairsL2);
  FindBinsL2 findL2(this->TopLevel);
  invoke(findL2,
         cellset,
         coords,
         this->LeafDimensions,
         this->LeafStartIndex,
         binCounts,
         binIds,
         this->CellIds);
  binCounts.ReleaseResources();

  // 11: From above, find the cells that each l2 bin intersects
  svtkm::cont::Algorithm::SortByKey(binIds, this->CellIds);
  svtkm::cont::Algorithm::ReduceByKey(binIds,
                                     svtkm::cont::make_ArrayHandleConstant(svtkm::Id(1), numPairsL2),
                                     bins,
                                     cellsPerBin,
                                     svtkm::Sum());
  binIds.ReleaseResources();

  // 12: Generate the leaf bin arrays
  svtkm::cont::ArrayHandle<svtkm::Id> cellsStart;
  svtkm::cont::Algorithm::ScanExclusive(cellsPerBin, cellsStart);

  svtkm::cont::ArrayCopy(svtkm::cont::ArrayHandleConstant<svtkm::Id>(0, numberOfLeaves),
                        this->CellStartIndex);
  svtkm::cont::ArrayCopy(svtkm::cont::ArrayHandleConstant<svtkm::Id>(0, numberOfLeaves),
                        this->CellCount);
  invoke(GenerateBinsL2{}, bins, cellsStart, cellsPerBin, this->CellStartIndex, this->CellCount);
}

//----------------------------------------------------------------------------
struct CellLocatorUniformBins::MakeExecObject
{
  template <typename CellSetType, typename DeviceAdapter>
  SVTKM_CONT void operator()(const CellSetType& cellSet,
                            DeviceAdapter,
                            const CellLocatorUniformBins& self) const
  {
    auto execObject =
      new svtkm::exec::CellLocatorUniformBins<CellSetType, DeviceAdapter>(self.TopLevel,
                                                                         self.LeafDimensions,
                                                                         self.LeafStartIndex,
                                                                         self.CellStartIndex,
                                                                         self.CellCount,
                                                                         self.CellIds,
                                                                         cellSet,
                                                                         self.GetCoordinates());
    self.ExecutionObjectHandle.Reset(execObject);
  }
};

struct CellLocatorUniformBins::PrepareForExecutionFunctor
{
  template <typename DeviceAdapter>
  SVTKM_CONT bool operator()(DeviceAdapter, const CellLocatorUniformBins& self) const
  {
    self.GetCellSet().CastAndCall(MakeExecObject{}, DeviceAdapter{}, self);
    return true;
  }
};

SVTKM_CONT const svtkm::exec::CellLocator* CellLocatorUniformBins::PrepareForExecution(
  svtkm::cont::DeviceAdapterId device) const
{
  if (!svtkm::cont::TryExecuteOnDevice(device, PrepareForExecutionFunctor(), *this))
  {
    throwFailedRuntimeDeviceTransfer("CellLocatorUniformBins", device);
  }
  return this->ExecutionObjectHandle.PrepareForExecution(device);
}

//----------------------------------------------------------------------------
void CellLocatorUniformBins::PrintSummary(std::ostream& out) const
{
  out << "DensityL1: " << this->DensityL1 << "\n";
  out << "DensityL2: " << this->DensityL2 << "\n";
  out << "Input CellSet: \n";
  this->GetCellSet().PrintSummary(out);
  out << "Input Coordinates: \n";
  this->GetCoordinates().PrintSummary(out);
  out << "LookupStructure:\n";
  out << "  TopLevelGrid\n";
  out << "    Dimensions: " << this->TopLevel.Dimensions << "\n";
  out << "    Origin: " << this->TopLevel.Origin << "\n";
  out << "    BinSize: " << this->TopLevel.BinSize << "\n";
  out << "  LeafDimensions:\n";
  svtkm::cont::printSummary_ArrayHandle(this->LeafDimensions, out);
  out << "  LeafStartIndex:\n";
  svtkm::cont::printSummary_ArrayHandle(this->LeafStartIndex, out);
  out << "  CellStartIndex:\n";
  svtkm::cont::printSummary_ArrayHandle(this->CellStartIndex, out);
  out << "  CellCount:\n";
  svtkm::cont::printSummary_ArrayHandle(this->CellCount, out);
  out << "  CellIds:\n";
  svtkm::cont::printSummary_ArrayHandle(this->CellIds, out);
}
}
} // svtkm::cont
