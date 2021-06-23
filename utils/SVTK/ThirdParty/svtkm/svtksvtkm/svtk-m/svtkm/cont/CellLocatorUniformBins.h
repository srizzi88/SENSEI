//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellLocatorUniformBins_h
#define svtk_m_cont_CellLocatorUniformBins_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/CellLocator.h>
#include <svtkm/cont/VirtualObjectHandle.h>

#include <svtkm/exec/CellInside.h>
#include <svtkm/exec/ParametricCoordinates.h>

#include <svtkm/Math.h>
#include <svtkm/Types.h>
#include <svtkm/VecFromPortalPermute.h>
#include <svtkm/VecTraits.h>

namespace svtkm
{
namespace internal
{
namespace cl_uniform_bins
{

using DimensionType = svtkm::Int16;
using DimVec3 = svtkm::Vec<DimensionType, 3>;
using FloatVec3 = svtkm::Vec3f;

struct Grid
{
  DimVec3 Dimensions;
  FloatVec3 Origin;
  FloatVec3 BinSize;
};

struct Bounds
{
  FloatVec3 Min;
  FloatVec3 Max;
};

SVTKM_EXEC inline svtkm::Id ComputeFlatIndex(const DimVec3& idx, const DimVec3 dim)
{
  return idx[0] + (dim[0] * (idx[1] + (dim[1] * idx[2])));
}

SVTKM_EXEC inline Grid ComputeLeafGrid(const DimVec3& idx, const DimVec3& dim, const Grid& l1Grid)
{
  return { dim,
           l1Grid.Origin + (static_cast<FloatVec3>(idx) * l1Grid.BinSize),
           l1Grid.BinSize / static_cast<FloatVec3>(dim) };
}

template <typename PointsVecType>
SVTKM_EXEC inline Bounds ComputeCellBounds(const PointsVecType& points)
{
  using CoordsType = typename svtkm::VecTraits<PointsVecType>::ComponentType;
  auto numPoints = svtkm::VecTraits<PointsVecType>::GetNumberOfComponents(points);

  CoordsType minp = points[0], maxp = points[0];
  for (svtkm::IdComponent i = 1; i < numPoints; ++i)
  {
    minp = svtkm::Min(minp, points[i]);
    maxp = svtkm::Max(maxp, points[i]);
  }

  return { FloatVec3(minp), FloatVec3(maxp) };
}
}
}
} // svtkm::internal::cl_uniform_bins

namespace svtkm
{
namespace exec
{

//--------------------------------------------------------------------
template <typename CellSetType, typename DeviceAdapter>
class SVTKM_ALWAYS_EXPORT CellLocatorUniformBins : public svtkm::exec::CellLocator
{
private:
  using DimVec3 = svtkm::internal::cl_uniform_bins::DimVec3;
  using FloatVec3 = svtkm::internal::cl_uniform_bins::FloatVec3;

  template <typename T>
  using ArrayPortalConst =
    typename svtkm::cont::ArrayHandle<T>::template ExecutionTypes<DeviceAdapter>::PortalConst;

  using CoordsPortalType =
    decltype(svtkm::cont::ArrayHandleVirtualCoordinates{}.PrepareForInput(DeviceAdapter{}));

  using CellSetP2CExecType =
    decltype(std::declval<CellSetType>().PrepareForInput(DeviceAdapter{},
                                                         svtkm::TopologyElementTagCell{},
                                                         svtkm::TopologyElementTagPoint{}));

  // TODO: This function may return false positives for non 3D cells as the
  // tests are done on the projection of the point on the cell. Extra checks
  // should be added to test if the point actually falls on the cell.
  template <typename CellShapeTag, typename CoordsType>
  SVTKM_EXEC static bool PointInsideCell(FloatVec3 point,
                                        CellShapeTag cellShape,
                                        CoordsType cellPoints,
                                        const svtkm::exec::FunctorBase& worklet,
                                        FloatVec3& parametricCoordinates)
  {
    auto bounds = svtkm::internal::cl_uniform_bins::ComputeCellBounds(cellPoints);
    if (point[0] >= bounds.Min[0] && point[0] <= bounds.Max[0] && point[1] >= bounds.Min[1] &&
        point[1] <= bounds.Max[1] && point[2] >= bounds.Min[2] && point[2] <= bounds.Max[2])
    {
      bool success = false;
      parametricCoordinates = svtkm::exec::WorldCoordinatesToParametricCoordinates(
        cellPoints, point, cellShape, success, worklet);
      return success && svtkm::exec::CellInside(parametricCoordinates, cellShape);
    }
    return false;
  }

public:
  SVTKM_CONT CellLocatorUniformBins(const svtkm::internal::cl_uniform_bins::Grid& topLevelGrid,
                                   const svtkm::cont::ArrayHandle<DimVec3>& leafDimensions,
                                   const svtkm::cont::ArrayHandle<svtkm::Id>& leafStartIndex,
                                   const svtkm::cont::ArrayHandle<svtkm::Id>& cellStartIndex,
                                   const svtkm::cont::ArrayHandle<svtkm::Id>& cellCount,
                                   const svtkm::cont::ArrayHandle<svtkm::Id>& cellIds,
                                   const CellSetType& cellSet,
                                   const svtkm::cont::CoordinateSystem& coords)
    : TopLevel(topLevelGrid)
    , LeafDimensions(leafDimensions.PrepareForInput(DeviceAdapter{}))
    , LeafStartIndex(leafStartIndex.PrepareForInput(DeviceAdapter{}))
    , CellStartIndex(cellStartIndex.PrepareForInput(DeviceAdapter{}))
    , CellCount(cellCount.PrepareForInput(DeviceAdapter{}))
    , CellIds(cellIds.PrepareForInput(DeviceAdapter{}))
    , CellSet(cellSet.PrepareForInput(DeviceAdapter{},
                                      svtkm::TopologyElementTagCell{},
                                      svtkm::TopologyElementTagPoint{}))
    , Coords(coords.GetData().PrepareForInput(DeviceAdapter{}))
  {
  }

  SVTKM_EXEC_CONT virtual ~CellLocatorUniformBins() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  SVTKM_EXEC
  void FindCell(const FloatVec3& point,
                svtkm::Id& cellId,
                FloatVec3& parametric,
                const svtkm::exec::FunctorBase& worklet) const override
  {
    using namespace svtkm::internal::cl_uniform_bins;

    cellId = -1;

    DimVec3 binId3 = static_cast<DimVec3>((point - this->TopLevel.Origin) / this->TopLevel.BinSize);
    if (binId3[0] >= 0 && binId3[0] < this->TopLevel.Dimensions[0] && binId3[1] >= 0 &&
        binId3[1] < this->TopLevel.Dimensions[1] && binId3[2] >= 0 &&
        binId3[2] < this->TopLevel.Dimensions[2])
    {
      svtkm::Id binId = ComputeFlatIndex(binId3, this->TopLevel.Dimensions);

      auto ldim = this->LeafDimensions.Get(binId);
      if (!ldim[0] || !ldim[1] || !ldim[2])
      {
        return;
      }

      auto leafGrid = ComputeLeafGrid(binId3, ldim, this->TopLevel);

      DimVec3 leafId3 = static_cast<DimVec3>((point - leafGrid.Origin) / leafGrid.BinSize);
      // precision issues may cause leafId3 to be out of range so clamp it
      leafId3 = svtkm::Max(DimVec3(0), svtkm::Min(ldim - DimVec3(1), leafId3));

      svtkm::Id leafStart = this->LeafStartIndex.Get(binId);
      svtkm::Id leafId = leafStart + ComputeFlatIndex(leafId3, leafGrid.Dimensions);

      svtkm::Id start = this->CellStartIndex.Get(leafId);
      svtkm::Id end = start + this->CellCount.Get(leafId);
      for (svtkm::Id i = start; i < end; ++i)
      {
        svtkm::Id cid = this->CellIds.Get(i);
        auto indices = this->CellSet.GetIndices(cid);
        auto pts = svtkm::make_VecFromPortalPermute(&indices, this->Coords);
        FloatVec3 pc;
        if (PointInsideCell(point, this->CellSet.GetCellShape(cid), pts, worklet, pc))
        {
          cellId = cid;
          parametric = pc;
          break;
        }
      }
    }
  }

private:
  svtkm::internal::cl_uniform_bins::Grid TopLevel;

  ArrayPortalConst<DimVec3> LeafDimensions;
  ArrayPortalConst<svtkm::Id> LeafStartIndex;

  ArrayPortalConst<svtkm::Id> CellStartIndex;
  ArrayPortalConst<svtkm::Id> CellCount;
  ArrayPortalConst<svtkm::Id> CellIds;

  CellSetP2CExecType CellSet;
  CoordsPortalType Coords;
};
}
} // svtkm::exec


namespace svtkm
{
namespace cont
{

//----------------------------------------------------------------------------
class SVTKM_CONT_EXPORT CellLocatorUniformBins : public svtkm::cont::CellLocator
{
public:
  CellLocatorUniformBins()
    : DensityL1(32.0f)
    , DensityL2(2.0f)
  {
  }

  /// Get/Set the desired approximate number of cells per level 1 bin
  ///
  void SetDensityL1(svtkm::FloatDefault val)
  {
    this->DensityL1 = val;
    this->SetModified();
  }
  svtkm::FloatDefault GetDensityL1() const { return this->DensityL1; }

  /// Get/Set the desired approximate number of cells per level 1 bin
  ///
  void SetDensityL2(svtkm::FloatDefault val)
  {
    this->DensityL2 = val;
    this->SetModified();
  }
  svtkm::FloatDefault GetDensityL2() const { return this->DensityL2; }

  void PrintSummary(std::ostream& out) const;

  const svtkm::exec::CellLocator* PrepareForExecution(
    svtkm::cont::DeviceAdapterId device) const override;

private:
  SVTKM_CONT void Build() override;

  svtkm::FloatDefault DensityL1, DensityL2;

  svtkm::internal::cl_uniform_bins::Grid TopLevel;
  svtkm::cont::ArrayHandle<svtkm::internal::cl_uniform_bins::DimVec3> LeafDimensions;
  svtkm::cont::ArrayHandle<svtkm::Id> LeafStartIndex;
  svtkm::cont::ArrayHandle<svtkm::Id> CellStartIndex;
  svtkm::cont::ArrayHandle<svtkm::Id> CellCount;
  svtkm::cont::ArrayHandle<svtkm::Id> CellIds;

  mutable svtkm::cont::VirtualObjectHandle<svtkm::exec::CellLocator> ExecutionObjectHandle;

  struct MakeExecObject;
  struct PrepareForExecutionFunctor;
};
}
} // svtkm::cont

#endif // svtk_m_cont_CellLocatorUniformBins_h
