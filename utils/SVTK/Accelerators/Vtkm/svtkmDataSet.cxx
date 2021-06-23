//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include "svtkmDataSet.h"

#include "svtkmDataArray.h"
#include "svtkmFilterPolicy.h"
#include "svtkmlib/ArrayConverters.h"

#include "svtkCell.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkNew.h"
#include "svtkPoints.h"

#include <svtkm/List.h>
#include <svtkm/cont/CellLocatorGeneral.h>
#include <svtkm/cont/DataSet.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/cont/PointLocator.h>
#include <svtkm/cont/PointLocatorUniformGrid.h>
#include <svtkm/worklet/ScatterPermutation.h>

#include <mutex>

namespace
{

using SupportedCellSets =
  svtkm::ListAppend<svtkmInputFilterPolicy::AllCellSetList, svtkmOutputFilterPolicy::AllCellSetList>;

template <typename LocatorControl>
struct VtkmLocator
{
  std::mutex lock;
  std::unique_ptr<LocatorControl> control;
  svtkMTimeType buildTime = 0;
};

} // anonymous

struct svtkmDataSet::DataMembers
{
  svtkm::cont::DynamicCellSet CellSet;
  svtkm::cont::CoordinateSystem Coordinates;
  svtkNew<svtkGenericCell> Cell;

  VtkmLocator<svtkm::cont::PointLocator> PointLocator;
  VtkmLocator<svtkm::cont::CellLocator> CellLocator;
};

//----------------------------------------------------------------------------
svtkmDataSet::svtkmDataSet()
  : Internals(new DataMembers)
{
}

svtkmDataSet::~svtkmDataSet() = default;

svtkStandardNewMacro(svtkmDataSet);

void svtkmDataSet::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->Internals->CellSet.PrintSummary(os);
  this->Internals->Coordinates.PrintSummary(os);
}

//----------------------------------------------------------------------------
void svtkmDataSet::SetVtkmDataSet(const svtkm::cont::DataSet& ds)
{
  this->Internals->CellSet = ds.GetCellSet();
  this->Internals->Coordinates = ds.GetCoordinateSystem();
  fromsvtkm::ConvertArrays(ds, this);
}

svtkm::cont::DataSet svtkmDataSet::GetVtkmDataSet() const
{
  svtkm::cont::DataSet ds;
  ds.SetCellSet(this->Internals->CellSet);
  ds.AddCoordinateSystem(this->Internals->Coordinates);
  tosvtkm::ProcessFields(const_cast<svtkmDataSet*>(this), ds, tosvtkm::FieldsFlag::PointsAndCells);

  return ds;
}

//----------------------------------------------------------------------------
void svtkmDataSet::CopyStructure(svtkDataSet* ds)
{
  auto svtkmds = svtkmDataSet::SafeDownCast(ds);
  if (svtkmds)
  {
    this->Initialize();
    this->Internals->CellSet = svtkmds->Internals->CellSet;
    this->Internals->Coordinates = svtkmds->Internals->Coordinates;
  }
}

svtkIdType svtkmDataSet::GetNumberOfPoints()
{
  return this->Internals->Coordinates.GetNumberOfPoints();
}

svtkIdType svtkmDataSet::GetNumberOfCells()
{
  auto* csBase = this->Internals->CellSet.GetCellSetBase();
  return csBase ? csBase->GetNumberOfCells() : 0;
}

double* svtkmDataSet::GetPoint(svtkIdType ptId)
{
  static double point[3];
  this->GetPoint(ptId, point);
  return point;
}

void svtkmDataSet::GetPoint(svtkIdType id, double x[3])
{
  auto portal = this->Internals->Coordinates.GetData().GetPortalConstControl();
  auto value = portal.Get(id);
  x[0] = value[0];
  x[1] = value[1];
  x[2] = value[2];
}

svtkCell* svtkmDataSet::GetCell(svtkIdType cellId)
{
  this->GetCell(cellId, this->Internals->Cell);
  return this->Internals->Cell->GetRepresentativeCell();
}

void svtkmDataSet::GetCell(svtkIdType cellId, svtkGenericCell* cell)
{
  cell->SetCellType(this->GetCellType(cellId));

  auto* pointIds = cell->GetPointIds();
  this->GetCellPoints(cellId, pointIds);

  auto numPoints = pointIds->GetNumberOfIds();
  cell->GetPoints()->SetNumberOfPoints(numPoints);
  for (svtkIdType i = 0; i < numPoints; ++i)
  {
    double x[3];
    this->GetPoint(pointIds->GetId(i), x);
    cell->GetPoints()->SetPoint(i, x);
  }
}

void svtkmDataSet::GetCellBounds(svtkIdType cellId, double bounds[6])
{
  if (this->Internals->Coordinates.GetData()
        .IsType<svtkm::cont::ArrayHandleUniformPointCoordinates>() &&
    this->Internals->CellSet.IsType<svtkm::cont::CellSetStructured<3> >())
  {
    auto portal = this->Internals->Coordinates.GetData()
                    .Cast<svtkm::cont::ArrayHandleUniformPointCoordinates>()
                    .GetPortalConstControl();

    svtkm::internal::ConnectivityStructuredInternals<3> helper;
    helper.SetPointDimensions(portal.GetDimensions());
    auto id3 = helper.FlatToLogicalCellIndex(cellId);
    auto min = portal.Get(id3);
    auto max = min + portal.GetSpacing();
    for (int i = 0; i < 3; ++i)
    {
      bounds[2 * i] = min[i];
      bounds[2 * i + 1] = max[i];
    }
  }
  else
  {
    Superclass::GetCellBounds(cellId, bounds);
  }
}

int svtkmDataSet::GetCellType(svtkIdType cellId)
{
  auto* csBase = this->Internals->CellSet.GetCellSetBase();
  if (csBase)
  {
    return csBase->GetCellShape(cellId);
  }
  return SVTK_EMPTY_CELL;
}

void svtkmDataSet::GetCellPoints(svtkIdType cellId, svtkIdList* ptIds)
{
  auto* csBase = this->Internals->CellSet.GetCellSetBase();
  if (csBase)
  {
    svtkm::Id numPoints = csBase->GetNumberOfPointsInCell(cellId);
    ptIds->SetNumberOfIds(numPoints);
    csBase->GetCellPointIds(cellId, ptIds->GetPointer(0));
  }
}

namespace
{

struct WorkletGetPointCells : svtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn);
  using ExecutionSignature = void(CellCount, CellIndices, Device);
  using ScatterType = svtkm::worklet::ScatterPermutation<>;

  explicit WorkletGetPointCells(svtkIdList* output)
    : Output(output)
  {
  }

  template <typename IndicesVecType>
  SVTKM_EXEC void operator()(svtkm::Id, IndicesVecType, svtkm::cont::DeviceAdapterTagCuda) const
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename IndicesVecType, typename Device>
  SVTKM_EXEC void operator()(svtkm::Id count, IndicesVecType idxs, Device) const
  {
    this->Output->SetNumberOfIds(count);
    for (svtkm::Id i = 0; i < count; ++i)
    {
      this->Output->SetId(i, idxs[i]);
    }
  }

  svtkIdList* Output;
};

} // anonymous namespace

void svtkmDataSet::GetPointCells(svtkIdType ptId, svtkIdList* cellIds)
{
  auto scatter = WorkletGetPointCells::ScatterType(svtkm::cont::make_ArrayHandle(&ptId, 1));
  svtkm::cont::Invoker invoke(svtkm::cont::DeviceAdapterTagSerial{});
  invoke(WorkletGetPointCells{ cellIds }, scatter,
    this->Internals->CellSet.ResetCellSetList(SupportedCellSets{}));
}

svtkIdType svtkmDataSet::FindPoint(double x[3])
{
  auto& locator = this->Internals->PointLocator;
  // critical section
  {
    std::lock_guard<std::mutex> lock(locator.lock);
    if (locator.buildTime < this->GetMTime())
    {
      locator.control.reset(new svtkm::cont::PointLocatorUniformGrid);
      locator.control->SetCoordinates(this->Internals->Coordinates);
      locator.control->Update();
      locator.buildTime = this->GetMTime();
    }
  }
  auto execLocator = locator.control->PrepareForExecution(svtkm::cont::DeviceAdapterTagSerial{});

  svtkm::Vec<svtkm::FloatDefault, 3> point(x[0], x[1], x[2]);
  svtkm::Id pointId = -1;
  svtkm::FloatDefault d2 = 0;
  // exec object created for the Serial device can be called directly
  execLocator->FindNearestNeighbor(point, pointId, d2);
  return pointId;
}

// non thread-safe version
svtkIdType svtkmDataSet::FindCell(
  double x[3], svtkCell*, svtkIdType, double, int& subId, double pcoords[3], double* weights)
{
  // just call the thread-safe version
  return this->FindCell(x, nullptr, nullptr, -1, 0.0, subId, pcoords, weights);
}

// thread-safe version
svtkIdType svtkmDataSet::FindCell(double x[3], svtkCell*, svtkGenericCell*, svtkIdType, double,
  int& subId, double pcoords[3], double* weights)
{
  auto& locator = this->Internals->CellLocator;
  // critical section
  {
    std::lock_guard<std::mutex> lock(locator.lock);
    if (locator.buildTime < this->GetMTime())
    {
      locator.control.reset(new svtkm::cont::CellLocatorGeneral);
      locator.control->SetCellSet(this->Internals->CellSet);
      locator.control->SetCoordinates(this->Internals->Coordinates);
      locator.control->Update();
      locator.buildTime = this->GetMTime();
    }
  }
  auto execLocator = locator.control->PrepareForExecution(svtkm::cont::DeviceAdapterTagSerial{});

  svtkm::Vec<svtkm::FloatDefault, 3> point(x[0], x[1], x[2]);
  svtkm::Vec<svtkm::FloatDefault, 3> pc;
  svtkm::Id cellId = -1;
  // exec object created for the Serial device can be called directly
  execLocator->FindCell(point, cellId, pc, svtkm::worklet::WorkletMapField{});

  if (cellId >= 0)
  {
    double closestPoint[3], dist2;
    svtkNew<svtkGenericCell> svtkcell;
    this->GetCell(cellId, svtkcell);
    svtkcell->EvaluatePosition(x, closestPoint, subId, pcoords, dist2, weights);
  }

  return cellId;
}

void svtkmDataSet::Squeeze()
{
  Superclass::Squeeze();

  this->Internals->PointLocator.control.reset(nullptr);
  this->Internals->PointLocator.buildTime = 0;
  this->Internals->CellLocator.control.reset(nullptr);
  this->Internals->CellLocator.buildTime = 0;
}

void svtkmDataSet::ComputeBounds()
{
  if (this->GetMTime() > this->ComputeTime)
  {
    svtkm::Bounds bounds = this->Internals->Coordinates.GetBounds();
    this->Bounds[0] = bounds.X.Min;
    this->Bounds[1] = bounds.X.Max;
    this->Bounds[2] = bounds.Y.Min;
    this->Bounds[3] = bounds.Y.Max;
    this->Bounds[4] = bounds.Z.Min;
    this->Bounds[5] = bounds.Z.Max;
    this->ComputeTime.Modified();
  }
}

void svtkmDataSet::Initialize()
{
  Superclass::Initialize();
  this->Internals = std::make_shared<DataMembers>();
}

namespace
{

struct MaxCellSize
{
  template <svtkm::IdComponent DIM>
  void operator()(
    const svtkm::cont::CellSetStructured<DIM>& cellset, svtkm::IdComponent& result) const
  {
    result = cellset.GetNumberOfPointsInCell(0);
  }

  template <typename S>
  void operator()(const svtkm::cont::CellSetSingleType<S>& cellset, svtkm::IdComponent& result) const
  {
    result = cellset.GetNumberOfPointsInCell(0);
  }

  template <typename S1, typename S2, typename S3>
  void operator()(
    const svtkm::cont::CellSetExplicit<S1, S2, S3>& cellset, svtkm::IdComponent& result) const
  {
    auto counts =
      cellset.GetNumIndicesArray(svtkm::TopologyElementTagCell{}, svtkm::TopologyElementTagPoint{});
    result = svtkm::cont::Algorithm::Reduce(counts, svtkm::IdComponent{ 0 }, svtkm::Maximum{});
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cellset, svtkm::IdComponent& result) const
  {
    result = -1;
    svtkm::Id numberOfCells = cellset.GetNumberOfCells();
    for (svtkm::Id i = 0; i < numberOfCells; ++i)
    {
      result = std::max(result, cellset.GetNumberOfPointsInCell(i));
    }
  }
};
} // anonymous namespace

int svtkmDataSet::GetMaxCellSize()
{
  svtkm::IdComponent result = 0;
  svtkm::cont::CastAndCall(
    this->Internals->CellSet.ResetCellSetList(SupportedCellSets{}), MaxCellSize{}, result);
  return result;
}

unsigned long svtkmDataSet::GetActualMemorySize()
{
  return this->Superclass::GetActualMemorySize();
}

void svtkmDataSet::ShallowCopy(svtkDataObject* src)
{
  auto obj = svtkmDataSet::SafeDownCast(src);
  if (obj)
  {
    Superclass::ShallowCopy(obj);
    this->Internals = obj->Internals;
  }
}

void svtkmDataSet::DeepCopy(svtkDataObject* src)
{
  auto other = svtkmDataSet::SafeDownCast(src);
  if (other)
  {
    auto* csBase = other->Internals->CellSet.GetCellSetBase();
    if (csBase)
    {
      this->Initialize();

      this->Internals->CellSet = other->Internals->CellSet.NewInstance();
      this->Internals->CellSet.GetCellSetBase()->DeepCopy(csBase);
    }
  }
}
