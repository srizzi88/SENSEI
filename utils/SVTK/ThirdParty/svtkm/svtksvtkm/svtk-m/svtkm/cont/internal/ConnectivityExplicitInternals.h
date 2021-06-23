//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_ConnectivityExplicitInternals_h
#define svtk_m_cont_internal_ConnectivityExplicitInternals_h

#include <svtkm/CellShape.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/internal/ReverseConnectivityBuilder.h>
#include <svtkm/exec/ExecutionWholeArray.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

template <typename ShapesStorageTag = SVTKM_DEFAULT_STORAGE_TAG,
          typename ConnectivityStorageTag = SVTKM_DEFAULT_STORAGE_TAG,
          typename OffsetsStorageTag = SVTKM_DEFAULT_STORAGE_TAG>
struct ConnectivityExplicitInternals
{
  using ShapesArrayType = svtkm::cont::ArrayHandle<svtkm::UInt8, ShapesStorageTag>;
  using ConnectivityArrayType = svtkm::cont::ArrayHandle<svtkm::Id, ConnectivityStorageTag>;
  using OffsetsArrayType = svtkm::cont::ArrayHandle<svtkm::Id, OffsetsStorageTag>;

  ShapesArrayType Shapes;
  ConnectivityArrayType Connectivity;
  OffsetsArrayType Offsets;

  bool ElementsValid;

  SVTKM_CONT
  ConnectivityExplicitInternals()
    : ElementsValid(false)
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfElements() const
  {
    SVTKM_ASSERT(this->ElementsValid);

    return this->Shapes.GetNumberOfValues();
  }

  SVTKM_CONT
  void ReleaseResourcesExecution()
  {
    this->Shapes.ReleaseResourcesExecution();
    this->Connectivity.ReleaseResourcesExecution();
    this->Offsets.ReleaseResourcesExecution();
  }

  SVTKM_CONT
  void PrintSummary(std::ostream& out) const
  {
    if (this->ElementsValid)
    {
      out << "     Shapes: ";
      svtkm::cont::printSummary_ArrayHandle(this->Shapes, out);
      out << "     Connectivity: ";
      svtkm::cont::printSummary_ArrayHandle(this->Connectivity, out);
      out << "     Offsets: ";
      svtkm::cont::printSummary_ArrayHandle(this->Offsets, out);
    }
    else
    {
      out << "     Not Allocated" << std::endl;
    }
  }
};

// Pass through (needed for ReverseConnectivityBuilder)
struct PassThrough
{
  SVTKM_EXEC svtkm::Id operator()(const svtkm::Id& val) const { return val; }
};

// Compute cell id from input connectivity:
// Find the upper bound of the conn idx in the offsets table and subtract 1
//
// Example:
// Offsets: |  0        |  3        |  6           |  10       |
// Conn:    |  0  1  2  |  0  1  3  |  2  4  5  6  |  1  3  5  |
// ConnIdx: |  0  1  2  |  3  4  5  |  6  7  8  9  |  10 11 12 |
// UpprBnd: |  1  1  1  |  2  2  2  |  3  3  3  3  |  4  4  4  |
// CellIdx: |  0  0  0  |  1  1  1  |  2  2  2  2  |  3  3  3  |
template <typename OffsetsPortalType>
struct ConnIdxToCellIdCalc
{
  OffsetsPortalType Offsets;

  SVTKM_CONT
  ConnIdxToCellIdCalc(const OffsetsPortalType& offsets)
    : Offsets(offsets)
  {
  }

  SVTKM_EXEC
  svtkm::Id operator()(svtkm::Id inIdx) const
  {
    // Compute the upper bound index:
    svtkm::Id upperBoundIdx;
    {
      svtkm::Id first = 0;
      svtkm::Id length = this->Offsets.GetNumberOfValues();

      while (length > 0)
      {
        svtkm::Id halfway = length / 2;
        svtkm::Id pos = first + halfway;
        svtkm::Id val = this->Offsets.Get(pos);
        if (val <= inIdx)
        {
          first = pos + 1;
          length -= halfway + 1;
        }
        else
        {
          length = halfway;
        }
      }

      upperBoundIdx = first;
    }

    return upperBoundIdx - 1;
  }
};

// Much easier for CellSetSingleType:
struct ConnIdxToCellIdCalcSingleType
{
  svtkm::IdComponent CellSize;

  SVTKM_CONT
  ConnIdxToCellIdCalcSingleType(svtkm::IdComponent cellSize)
    : CellSize(cellSize)
  {
  }

  SVTKM_EXEC
  svtkm::Id operator()(svtkm::Id inIdx) const { return inIdx / this->CellSize; }
};

template <typename ConnTableT, typename RConnTableT, typename Device>
void ComputeRConnTable(RConnTableT& rConnTable,
                       const ConnTableT& connTable,
                       svtkm::Id numberOfPoints,
                       Device)
{
  if (rConnTable.ElementsValid)
  {
    return;
  }

  const auto& conn = connTable.Connectivity;
  auto& rConn = rConnTable.Connectivity;
  auto& rOffsets = rConnTable.Offsets;
  const svtkm::Id rConnSize = conn.GetNumberOfValues();

  const auto offInPortal = connTable.Offsets.PrepareForInput(Device{});

  PassThrough idxCalc{};
  ConnIdxToCellIdCalc<decltype(offInPortal)> cellIdCalc{ offInPortal };

  svtkm::cont::internal::ReverseConnectivityBuilder builder;
  builder.Run(conn, rConn, rOffsets, idxCalc, cellIdCalc, numberOfPoints, rConnSize, Device());

  rConnTable.Shapes = svtkm::cont::make_ArrayHandleConstant(
    static_cast<svtkm::UInt8>(CELL_SHAPE_VERTEX), numberOfPoints);
  rConnTable.ElementsValid = true;
}

// Specialize for CellSetSingleType:
template <typename RConnTableT, typename ConnectivityStorageTag, typename Device>
void ComputeRConnTable(RConnTableT& rConnTable,
                       const ConnectivityExplicitInternals< // SingleType specialization types:
                         typename svtkm::cont::ArrayHandleConstant<svtkm::UInt8>::StorageTag,
                         ConnectivityStorageTag,
                         typename svtkm::cont::ArrayHandleCounting<svtkm::Id>::StorageTag>& connTable,
                       svtkm::Id numberOfPoints,
                       Device)
{
  if (rConnTable.ElementsValid)
  {
    return;
  }

  const auto& conn = connTable.Connectivity;
  auto& rConn = rConnTable.Connectivity;
  auto& rOffsets = rConnTable.Offsets;
  const svtkm::Id rConnSize = conn.GetNumberOfValues();

  const svtkm::IdComponent cellSize = [&]() -> svtkm::IdComponent {
    if (connTable.Offsets.GetNumberOfValues() >= 2)
    {
      const auto firstTwo = svtkm::cont::ArrayGetValues({ 0, 1 }, connTable.Offsets);
      return static_cast<svtkm::IdComponent>(firstTwo[1] - firstTwo[0]);
    }
    return 0;
  }();

  PassThrough idxCalc{};
  ConnIdxToCellIdCalcSingleType cellIdCalc{ cellSize };

  svtkm::cont::internal::ReverseConnectivityBuilder builder;
  builder.Run(conn, rConn, rOffsets, idxCalc, cellIdCalc, numberOfPoints, rConnSize, Device());

  rConnTable.Shapes = svtkm::cont::make_ArrayHandleConstant(
    static_cast<svtkm::UInt8>(CELL_SHAPE_VERTEX), numberOfPoints);
  rConnTable.ElementsValid = true;
}
}
}
} // namespace svtkm::cont::internal

#endif //svtk_m_cont_internal_ConnectivityExplicitInternals_h
