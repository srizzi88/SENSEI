//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================
#include "CellSetConverters.h"

#include "ArrayConverters.hxx"
#include "DataSetConverters.h"

#include "svtkmFilterPolicy.h"

#include <svtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/cont/tbb/DeviceAdapterTBB.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/worklet/DispatcherMapTopology.h>

#include "svtkCellArray.h"
#include "svtkCellType.h"
#include "svtkIdTypeArray.h"
#include "svtkNew.h"
#include "svtkUnsignedCharArray.h"

namespace tosvtkm
{

namespace
{

template <typename PortalT>
struct ReorderHex : public svtkm::exec::FunctorBase
{
  ReorderHex(PortalT portal)
    : Portal{ portal }
  {
  }

  void operator()(svtkm::Id index) const
  {
    const svtkm::Id offset = index * 8;

    auto doSwap = [&](svtkm::Id id1, svtkm::Id id2) {
      id1 += offset;
      id2 += offset;

      const auto t = this->Portal.Get(id1);
      this->Portal.Set(id1, this->Portal.Get(id2));
      this->Portal.Set(id2, t);
    };

    doSwap(2, 3);
    doSwap(6, 7);
  }

  PortalT Portal;
};

struct RunReorder
{
  RunReorder(svtkm::cont::ArrayHandle<svtkm::Id>& handle)
    : Handle(handle)
  {
  }

  template <typename Device>
  bool operator()(Device) const
  {
    using Algo = typename svtkm::cont::DeviceAdapterAlgorithm<Device>;

    auto portal = this->Handle.PrepareForInPlace(Device{});

    using Functor = ReorderHex<decltype(portal)>;
    Functor reorder{ portal };

    Algo::Schedule(reorder, portal.GetNumberOfValues() / 8);
    return true;
  }

  svtkm::cont::ArrayHandle<svtkm::Id>& Handle;
};

struct BuildSingleTypeCellSetVisitor
{
  template <typename CellStateT>
  svtkm::cont::DynamicCellSet operator()(
    CellStateT& state, svtkm::UInt8 cellType, svtkm::IdComponent cellSize, svtkIdType numPoints)
  {
    using SVTKIdT = typename CellStateT::ValueType; // might not be svtkIdType...
    using SVTKArrayT = svtkAOSDataArrayTemplate<SVTKIdT>;
    static constexpr bool IsVtkmIdType = std::is_same<SVTKIdT, svtkm::Id>::value;

    // Construct an arrayhandle that holds the connectivity array
    using DirectConverter = tosvtkm::DataArrayToArrayHandle<SVTKArrayT, 1>;
    auto connHandleDirect = DirectConverter::Wrap(state.GetConnectivity());

    // Cast if necessary:
    auto connHandle = IsVtkmIdType ? connHandleDirect
                                   : svtkm::cont::make_ArrayHandleCast<svtkm::Id>(connHandleDirect);

    using ConnHandleType = typename std::decay<decltype(connHandle)>::type;
    using ConnStorageTag = typename ConnHandleType::StorageTag;
    using CellSetType = svtkm::cont::CellSetSingleType<ConnStorageTag>;

    CellSetType cellSet;
    cellSet.Fill(static_cast<svtkm::Id>(numPoints), cellType, cellSize, connHandle);
    return cellSet;
  }
};

struct BuildSingleTypeVoxelCellSetVisitor
{
  template <typename CellStateT>
  svtkm::cont::DynamicCellSet operator()(CellStateT& state, svtkIdType numPoints)
  {
    svtkm::cont::ArrayHandle<svtkm::Id> connHandle;
    {
      auto* conn = state.GetConnectivity();
      const auto* origData = conn->GetPointer(0);
      const svtkm::Id numIds = conn->GetNumberOfValues();
      svtkm::cont::ArrayCopy(
        svtkm::cont::make_ArrayHandle(origData, numIds, svtkm::CopyFlag::Off), connHandle);

      // reorder cells from voxel->hex: which only can run on
      // devices that have shared memory / vtable with the CPU
      using SMPTypes = svtkm::List<svtkm::cont::DeviceAdapterTagTBB,
        svtkm::cont::DeviceAdapterTagOpenMP, svtkm::cont::DeviceAdapterTagSerial>;

      RunReorder reorder{ connHandle };
      svtkm::cont::TryExecute(reorder, SMPTypes{});
    }

    using CellSetType = svtkm::cont::CellSetSingleType<>;

    CellSetType cellSet;
    cellSet.Fill(numPoints, svtkm::CELL_SHAPE_HEXAHEDRON, 8, connHandle);
    return cellSet;
  }
};

} // end anon namespace

// convert a cell array of a single type to a svtkm CellSetSingleType
svtkm::cont::DynamicCellSet ConvertSingleType(
  svtkCellArray* cells, int cellType, svtkIdType numberOfPoints)
{
  switch (cellType)
  {
    case SVTK_LINE:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, svtkm::CELL_SHAPE_LINE, 2, numberOfPoints);

    case SVTK_HEXAHEDRON:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, svtkm::CELL_SHAPE_HEXAHEDRON, 8, numberOfPoints);

    case SVTK_VOXEL:
      // Note that this is a special case that reorders ids voxel to hex:
      return cells->Visit(BuildSingleTypeVoxelCellSetVisitor{}, numberOfPoints);

    case SVTK_QUAD:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, svtkm::CELL_SHAPE_QUAD, 4, numberOfPoints);

    case SVTK_TETRA:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, svtkm::CELL_SHAPE_TETRA, 4, numberOfPoints);

    case SVTK_TRIANGLE:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, svtkm::CELL_SHAPE_TRIANGLE, 3, numberOfPoints);

    case SVTK_VERTEX:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, svtkm::CELL_SHAPE_VERTEX, 1, numberOfPoints);

    case SVTK_WEDGE:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, svtkm::CELL_SHAPE_WEDGE, 6, numberOfPoints);

    case SVTK_PYRAMID:
      return cells->Visit(
        BuildSingleTypeCellSetVisitor{}, svtkm::CELL_SHAPE_PYRAMID, 5, numberOfPoints);

    default:
      break;
  }

  throw svtkm::cont::ErrorBadType("Unsupported SVTK cell type in "
                                 "CellSetSingleType converter.");
}

namespace
{

struct BuildExplicitCellSetVisitor
{
  template <typename CellStateT, typename S>
  svtkm::cont::DynamicCellSet operator()(CellStateT& state,
    const svtkm::cont::ArrayHandle<svtkm::UInt8, S>& shapes, svtkm::Id numPoints) const
  {
    using SVTKIdT = typename CellStateT::ValueType; // might not be svtkIdType...
    using SVTKArrayT = svtkAOSDataArrayTemplate<SVTKIdT>;
    static constexpr bool IsVtkmIdType = std::is_same<SVTKIdT, svtkm::Id>::value;

    // Construct arrayhandles to hold the arrays
    using DirectConverter = tosvtkm::DataArrayToArrayHandle<SVTKArrayT, 1>;
    auto offsetsHandleDirect = DirectConverter::Wrap(state.GetOffsets());
    auto connHandleDirect = DirectConverter::Wrap(state.GetConnectivity());

    // Cast if necessary:
    auto connHandle = IsVtkmIdType ? connHandleDirect
                                   : svtkm::cont::make_ArrayHandleCast<svtkm::Id>(connHandleDirect);
    auto offsetsHandle = IsVtkmIdType
      ? offsetsHandleDirect
      : svtkm::cont::make_ArrayHandleCast<svtkm::Id>(offsetsHandleDirect);

    using ShapesStorageTag = typename std::decay<decltype(shapes)>::type::StorageTag;
    using ConnStorageTag = typename decltype(connHandle)::StorageTag;
    using OffsetsStorageTag = typename decltype(offsetsHandle)::StorageTag;
    using CellSetType =
      svtkm::cont::CellSetExplicit<ShapesStorageTag, ConnStorageTag, OffsetsStorageTag>;

    CellSetType cellSet;
    cellSet.Fill(numPoints, shapes, connHandle, offsetsHandle);
    return cellSet;
  }
};

} // end anon namespace

// convert a cell array of mixed types to a svtkm CellSetExplicit
svtkm::cont::DynamicCellSet Convert(
  svtkUnsignedCharArray* types, svtkCellArray* cells, svtkIdType numberOfPoints)
{
  using ShapeArrayType = svtkAOSDataArrayTemplate<svtkm::UInt8>;
  using ShapeConverter = tosvtkm::DataArrayToArrayHandle<ShapeArrayType, 1>;
  return cells->Visit(BuildExplicitCellSetVisitor{}, ShapeConverter::Wrap(types), numberOfPoints);
}

} // namespace tosvtkm

namespace fromsvtkm
{

bool Convert(const svtkm::cont::DynamicCellSet& toConvert, svtkCellArray* cells,
  svtkUnsignedCharArray* typesArray)
{
  const auto* cellset = toConvert.GetCellSetBase();

  // small hack as we can't compute properly the number of cells
  // instead we will pre-allocate and than shrink
  const svtkm::Id numCells = cellset->GetNumberOfCells();
  const svtkm::Id maxSize = numCells * 8; // largest cell type is hex

  // TODO this could steal the guts out of explicit cellsets as a future
  // no-copy optimization.
  svtkNew<svtkIdTypeArray> offsetsArray;
  offsetsArray->SetNumberOfTuples(static_cast<svtkIdType>(numCells + 1));
  svtkNew<svtkIdTypeArray> connArray;
  connArray->SetNumberOfTuples(static_cast<svtkIdType>(maxSize));

  if (typesArray)
  {
    typesArray->SetNumberOfComponents(1);
    typesArray->SetNumberOfTuples(static_cast<svtkIdType>(numCells));
  }

  svtkIdType* connIter = connArray->GetPointer(0);
  const svtkIdType* connBegin = connIter;

  for (svtkm::Id cellId = 0; cellId < numCells; ++cellId)
  {
    const svtkIdType svtkCellId = static_cast<svtkIdType>(cellId);
    const svtkm::Id npts = cellset->GetNumberOfPointsInCell(cellId);
    assert(npts <= 8 && "Initial allocation assumes no more than 8 pts/cell.");

    const svtkIdType offset = static_cast<svtkIdType>(connIter - connBegin);
    offsetsArray->SetValue(svtkCellId, offset);

    cellset->GetCellPointIds(cellId, connIter);
    connIter += npts;

    if (typesArray)
    {
      typesArray->SetValue(svtkCellId, cellset->GetCellShape(cellId));
    }
  }

  const svtkIdType connSize = static_cast<svtkIdType>(connIter - connBegin);
  offsetsArray->SetValue(static_cast<svtkIdType>(numCells), connSize);
  connArray->Resize(connSize);
  cells->SetData(offsetsArray, connArray);

  return true;
}

} // namespace fromsvtkm
