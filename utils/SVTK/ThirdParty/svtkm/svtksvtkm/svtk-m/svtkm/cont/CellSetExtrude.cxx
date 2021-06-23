//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/CellSetExtrude.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/DeviceAdapter.h>

#include <svtkm/CellShape.h>

namespace svtkm
{
namespace cont
{

CellSetExtrude::CellSetExtrude()
  : svtkm::cont::CellSet()
  , IsPeriodic(false)
  , NumberOfPointsPerPlane(0)
  , NumberOfCellsPerPlane(0)
  , NumberOfPlanes(0)
  , ReverseConnectivityBuilt(false)
{
}

CellSetExtrude::CellSetExtrude(const svtkm::cont::ArrayHandle<svtkm::Int32>& conn,
                               svtkm::Int32 numberOfPointsPerPlane,
                               svtkm::Int32 numberOfPlanes,
                               const svtkm::cont::ArrayHandle<svtkm::Int32>& nextNode,
                               bool periodic)
  : svtkm::cont::CellSet()
  , IsPeriodic(periodic)
  , NumberOfPointsPerPlane(numberOfPointsPerPlane)
  , NumberOfCellsPerPlane(
      static_cast<svtkm::Int32>(conn.GetNumberOfValues() / static_cast<svtkm::Id>(3)))
  , NumberOfPlanes(numberOfPlanes)
  , Connectivity(conn)
  , NextNode(nextNode)
  , ReverseConnectivityBuilt(false)
{
}


CellSetExtrude::CellSetExtrude(const CellSetExtrude& src)
  : CellSet(src)
  , IsPeriodic(src.IsPeriodic)
  , NumberOfPointsPerPlane(src.NumberOfPointsPerPlane)
  , NumberOfCellsPerPlane(src.NumberOfCellsPerPlane)
  , NumberOfPlanes(src.NumberOfPlanes)
  , Connectivity(src.Connectivity)
  , NextNode(src.NextNode)
  , ReverseConnectivityBuilt(src.ReverseConnectivityBuilt)
  , RConnectivity(src.RConnectivity)
  , ROffsets(src.ROffsets)
  , RCounts(src.RCounts)
  , PrevNode(src.PrevNode)
{
}

CellSetExtrude::CellSetExtrude(CellSetExtrude&& src) noexcept
  : CellSet(std::forward<CellSet>(src)),
    IsPeriodic(src.IsPeriodic),
    NumberOfPointsPerPlane(src.NumberOfPointsPerPlane),
    NumberOfCellsPerPlane(src.NumberOfCellsPerPlane),
    NumberOfPlanes(src.NumberOfPlanes),
    Connectivity(std::move(src.Connectivity)),
    NextNode(std::move(src.NextNode)),
    ReverseConnectivityBuilt(src.ReverseConnectivityBuilt),
    RConnectivity(std::move(src.RConnectivity)),
    ROffsets(std::move(src.ROffsets)),
    RCounts(std::move(src.RCounts)),
    PrevNode(std::move(src.PrevNode))
{
}

CellSetExtrude& CellSetExtrude::operator=(const CellSetExtrude& src)
{
  this->CellSet::operator=(src);

  this->IsPeriodic = src.IsPeriodic;
  this->NumberOfPointsPerPlane = src.NumberOfPointsPerPlane;
  this->NumberOfCellsPerPlane = src.NumberOfCellsPerPlane;
  this->NumberOfPlanes = src.NumberOfPlanes;
  this->Connectivity = src.Connectivity;
  this->NextNode = src.NextNode;
  this->ReverseConnectivityBuilt = src.ReverseConnectivityBuilt;
  this->RConnectivity = src.RConnectivity;
  this->ROffsets = src.ROffsets;
  this->RCounts = src.RCounts;
  this->PrevNode = src.PrevNode;

  return *this;
}

CellSetExtrude& CellSetExtrude::operator=(CellSetExtrude&& src) noexcept
{
  this->CellSet::operator=(std::forward<CellSet>(src));

  this->IsPeriodic = src.IsPeriodic;
  this->NumberOfPointsPerPlane = src.NumberOfPointsPerPlane;
  this->NumberOfCellsPerPlane = src.NumberOfCellsPerPlane;
  this->NumberOfPlanes = src.NumberOfPlanes;
  this->Connectivity = std::move(src.Connectivity);
  this->NextNode = std::move(src.NextNode);
  this->ReverseConnectivityBuilt = src.ReverseConnectivityBuilt;
  this->RConnectivity = std::move(src.RConnectivity);
  this->ROffsets = std::move(src.ROffsets);
  this->RCounts = std::move(src.RCounts);
  this->PrevNode = std::move(src.PrevNode);

  return *this;
}

CellSetExtrude::~CellSetExtrude()
{
}

svtkm::Int32 CellSetExtrude::GetNumberOfPlanes() const
{
  return this->NumberOfPlanes;
}

svtkm::Id CellSetExtrude::GetNumberOfCells() const
{
  if (this->IsPeriodic)
  {
    return static_cast<svtkm::Id>(this->NumberOfPlanes) *
      static_cast<svtkm::Id>(this->NumberOfCellsPerPlane);
  }
  else
  {
    return static_cast<svtkm::Id>(this->NumberOfPlanes - 1) *
      static_cast<svtkm::Id>(this->NumberOfCellsPerPlane);
  }
}

svtkm::Id CellSetExtrude::GetNumberOfPoints() const
{
  return static_cast<svtkm::Id>(this->NumberOfPlanes) *
    static_cast<svtkm::Id>(this->NumberOfPointsPerPlane);
}

svtkm::Id CellSetExtrude::GetNumberOfFaces() const
{
  return -1;
}

svtkm::Id CellSetExtrude::GetNumberOfEdges() const
{
  return -1;
}

svtkm::UInt8 CellSetExtrude::GetCellShape(svtkm::Id) const
{
  return svtkm::CellShapeTagWedge::Id;
}

svtkm::IdComponent CellSetExtrude::GetNumberOfPointsInCell(svtkm::Id) const
{
  return 6;
}

void CellSetExtrude::GetCellPointIds(svtkm::Id id, svtkm::Id* ptids) const
{
  auto conn = this->PrepareForInput(svtkm::cont::DeviceAdapterTagSerial{},
                                    svtkm::TopologyElementTagCell{},
                                    svtkm::TopologyElementTagPoint{});
  auto indices = conn.GetIndices(id);
  for (int i = 0; i < 6; ++i)
  {
    ptids[i] = indices[i];
  }
}

std::shared_ptr<CellSet> CellSetExtrude::NewInstance() const
{
  return std::make_shared<CellSetExtrude>();
}

void CellSetExtrude::DeepCopy(const CellSet* src)
{
  const auto* other = dynamic_cast<const CellSetExtrude*>(src);
  if (!other)
  {
    throw svtkm::cont::ErrorBadType("CellSetExplicit::DeepCopy types don't match");
  }

  this->IsPeriodic = other->IsPeriodic;

  this->NumberOfPointsPerPlane = other->NumberOfPointsPerPlane;
  this->NumberOfCellsPerPlane = other->NumberOfCellsPerPlane;
  this->NumberOfPlanes = other->NumberOfPlanes;

  svtkm::cont::ArrayCopy(other->Connectivity, this->Connectivity);
  svtkm::cont::ArrayCopy(other->NextNode, this->NextNode);

  this->ReverseConnectivityBuilt = other->ReverseConnectivityBuilt;

  if (this->ReverseConnectivityBuilt)
  {
    svtkm::cont::ArrayCopy(other->RConnectivity, this->RConnectivity);
    svtkm::cont::ArrayCopy(other->ROffsets, this->ROffsets);
    svtkm::cont::ArrayCopy(other->RCounts, this->RCounts);
    svtkm::cont::ArrayCopy(other->PrevNode, this->PrevNode);
  }
}

void CellSetExtrude::ReleaseResourcesExecution()
{
  this->Connectivity.ReleaseResourcesExecution();
  this->NextNode.ReleaseResourcesExecution();

  this->RConnectivity.ReleaseResourcesExecution();
  this->ROffsets.ReleaseResourcesExecution();
  this->RCounts.ReleaseResourcesExecution();
  this->PrevNode.ReleaseResourcesExecution();
}

svtkm::Id2 CellSetExtrude::GetSchedulingRange(svtkm::TopologyElementTagCell) const
{
  if (this->IsPeriodic)
  {
    return svtkm::Id2(this->NumberOfCellsPerPlane, this->NumberOfPlanes);
  }
  else
  {
    return svtkm::Id2(this->NumberOfCellsPerPlane, this->NumberOfPlanes - 1);
  }
}

svtkm::Id2 CellSetExtrude::GetSchedulingRange(svtkm::TopologyElementTagPoint) const
{
  return svtkm::Id2(this->NumberOfPointsPerPlane, this->NumberOfPlanes);
}

void CellSetExtrude::PrintSummary(std::ostream& out) const
{
  out << "   svtkmCellSetSingleType: " << std::endl;
  out << "   NumberOfCellsPerPlane: " << this->NumberOfCellsPerPlane << std::endl;
  out << "   NumberOfPointsPerPlane: " << this->NumberOfPointsPerPlane << std::endl;
  out << "   NumberOfPlanes: " << this->NumberOfPlanes << std::endl;
  out << "   Connectivity: " << std::endl;
  svtkm::cont::printSummary_ArrayHandle(this->Connectivity, out);
  out << "   NextNode: " << std::endl;
  svtkm::cont::printSummary_ArrayHandle(this->NextNode, out);
  out << "   ReverseConnectivityBuilt: " << this->NumberOfPlanes << std::endl;
}
}
} // svtkm::cont
