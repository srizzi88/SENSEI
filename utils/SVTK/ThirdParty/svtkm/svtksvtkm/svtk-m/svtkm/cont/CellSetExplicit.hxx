//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellSetExplicit_hxx
#define svtk_m_cont_CellSetExplicit_hxx

#include <svtkm/cont/CellSetExplicit.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayGetValues.h>
#include <svtkm/cont/ArrayHandleDecorator.h>
#include <svtkm/cont/Logging.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>
#include <svtkm/cont/TryExecute.h>

// This file uses a lot of very verbose identifiers and the clang formatted
// code quickly becomes unreadable. Stick with manual formatting for now.
//
// clang-format off

namespace svtkm
{
namespace cont
{

template <typename SST, typename CST, typename OST>
SVTKM_CONT
CellSetExplicit<SST, CST, OST>::CellSetExplicit()
  : CellSet()
  , Data(std::make_shared<Internals>())
{
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
CellSetExplicit<SST, CST, OST>::CellSetExplicit(const Thisclass& src)
  : CellSet(src)
  , Data(src.Data)
{
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
CellSetExplicit<SST, CST, OST>::CellSetExplicit(Thisclass &&src) noexcept
  : CellSet(std::forward<CellSet>(src))
  , Data(std::move(src.Data))
{
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
auto CellSetExplicit<SST, CST, OST>::operator=(const Thisclass& src)
-> Thisclass&
{
  this->CellSet::operator=(src);
  this->Data = src.Data;
  return *this;
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
auto CellSetExplicit<SST, CST, OST>::operator=(Thisclass&& src) noexcept
-> Thisclass&
{
  this->CellSet::operator=(std::forward<CellSet>(src));
  this->Data = std::move(src.Data);
  return *this;
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
CellSetExplicit<SST, CST, OST>::~CellSetExplicit()
{
  // explicitly define instead of '=default' to workaround an intel compiler bug
  // (see #179)
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>::PrintSummary(std::ostream& out) const
{
  out << "   ExplicitCellSet:" << std::endl;
  out << "   CellPointIds:" << std::endl;
  this->Data->CellPointIds.PrintSummary(out);
  out << "   PointCellIds:" << std::endl;
  this->Data->PointCellIds.PrintSummary(out);
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>::ReleaseResourcesExecution()
{
  this->Data->CellPointIds.ReleaseResourcesExecution();
  this->Data->PointCellIds.ReleaseResourcesExecution();
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
svtkm::Id CellSetExplicit<SST, CST, OST>::GetNumberOfCells() const
{
  return this->Data->CellPointIds.GetNumberOfElements();
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
svtkm::Id CellSetExplicit<SST, CST, OST>::GetNumberOfPoints() const
{
  return this->Data->NumberOfPoints;
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
svtkm::Id CellSetExplicit<SST, CST, OST>::GetNumberOfFaces() const
{
  return -1;
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
svtkm::Id CellSetExplicit<SST, CST, OST>::GetNumberOfEdges() const
{
  return -1;
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>::GetCellPointIds(svtkm::Id cellId,
                                                     svtkm::Id* ptids) const
{
  const auto offPortal = this->Data->CellPointIds.Offsets.GetPortalConstControl();
  const svtkm::Id start = offPortal.Get(cellId);
  const svtkm::Id end = offPortal.Get(cellId + 1);
  const svtkm::IdComponent numIndices = static_cast<svtkm::IdComponent>(end - start);
  auto connPortal = this->Data->CellPointIds.Connectivity.GetPortalConstControl();
  for (svtkm::IdComponent i = 0; i < numIndices; i++)
  {
    ptids[i] = connPortal.Get(start + i);
  }
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
SVTKM_CONT
svtkm::Id CellSetExplicit<SST, CST, OST>
::GetSchedulingRange(svtkm::TopologyElementTagCell) const
{
  return this->GetNumberOfCells();
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
svtkm::Id CellSetExplicit<SST, CST, OST>
::GetSchedulingRange(svtkm::TopologyElementTagPoint) const
{
  return this->GetNumberOfPoints();
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
svtkm::IdComponent CellSetExplicit<SST, CST, OST>
::GetNumberOfPointsInCell(svtkm::Id cellid) const
{
  const auto portal = this->Data->CellPointIds.Offsets.GetPortalConstControl();
  return static_cast<svtkm::IdComponent>(portal.Get(cellid + 1) -
                                        portal.Get(cellid));
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
svtkm::UInt8 CellSetExplicit<SST, CST, OST>
::GetCellShape(svtkm::Id cellid) const
{
  return this->Data->CellPointIds.Shapes.GetPortalConstControl().Get(cellid);
}

template <typename SST, typename CST, typename OST>
template <svtkm::IdComponent NumVecIndices>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>
::GetIndices(svtkm::Id cellId, svtkm::Vec<svtkm::Id, NumVecIndices>& ids) const
{
  const auto offPortal = this->Data->CellPointIds.Offsets.GetPortalConstControl();
  const svtkm::Id start = offPortal.Get(cellId);
  const svtkm::Id end = offPortal.Get(cellId + 1);
  const auto numCellIndices = static_cast<svtkm::IdComponent>(end - start);
  const auto connPortal = this->Data->CellPointIds.Connectivity.GetPortalConstControl();

  SVTKM_LOG_IF_S(svtkm::cont::LogLevel::Warn,
                numCellIndices != NumVecIndices,
                "GetIndices given a " << NumVecIndices
                << "-vec to fetch a cell with " << numCellIndices << "points. "
                "Truncating result.");

  const svtkm::IdComponent numIndices = svtkm::Min(NumVecIndices, numCellIndices);

  for (svtkm::IdComponent i = 0; i < numIndices; i++)
  {
    ids[i] = connPortal.Get(start + i);
  }
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>
::GetIndices(svtkm::Id cellId, svtkm::cont::ArrayHandle<svtkm::Id>& ids) const
{
  const auto offPortal = this->Data->CellPointIds.Offsets.GetPortalConstControl();
  const svtkm::Id start = offPortal.Get(cellId);
  const svtkm::Id end = offPortal.Get(cellId + 1);
  const svtkm::IdComponent numIndices = static_cast<svtkm::IdComponent>(end - start);
  ids.Allocate(numIndices);
  auto connPortal = this->Data->CellPointIds.Connectivity.GetPortalConstControl();

  auto outIdPortal = ids.GetPortalControl();

  for (svtkm::IdComponent i = 0; i < numIndices; i++)
  {
    outIdPortal.Set(i, connPortal.Get(start + i));
  }
}


//----------------------------------------------------------------------------
namespace internal
{

// Sets the first value of the array to zero if the handle is writable,
// otherwise do nothing:
template <typename ArrayType>
typename std::enable_if<svtkm::cont::internal::IsWritableArrayHandle<ArrayType>::value>::type
SetFirstToZeroIfWritable(ArrayType&& array)
{
  using ValueType = typename std::decay<ArrayType>::type::ValueType;
  using Traits = svtkm::TypeTraits<ValueType>;
  array.GetPortalControl().Set(0, Traits::ZeroInitialization());
}

template <typename ArrayType>
typename std::enable_if<!svtkm::cont::internal::IsWritableArrayHandle<ArrayType>::value>::type
SetFirstToZeroIfWritable(ArrayType&&)
{ /* no-op */ }

} // end namespace internal

template <typename SST, typename CST, typename OST>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>
::PrepareToAddCells(svtkm::Id numCells,
                    svtkm::Id connectivityMaxLen)
{
  this->Data->CellPointIds.Shapes.Allocate(numCells);
  this->Data->CellPointIds.Connectivity.Allocate(connectivityMaxLen);
  this->Data->CellPointIds.Offsets.Allocate(numCells + 1);
  internal::SetFirstToZeroIfWritable(this->Data->CellPointIds.Offsets);
  this->Data->NumberOfCellsAdded = 0;
  this->Data->ConnectivityAdded = 0;
}

template <typename SST, typename CST, typename OST>
template <typename IdVecType>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>::AddCell(svtkm::UInt8 cellType,
                                             svtkm::IdComponent numVertices,
                                             const IdVecType& ids)
{
  using Traits = svtkm::VecTraits<IdVecType>;
  SVTKM_STATIC_ASSERT_MSG((std::is_same<typename Traits::ComponentType, svtkm::Id>::value),
                         "CellSetSingleType::AddCell requires svtkm::Id for indices.");

  if (Traits::GetNumberOfComponents(ids) < numVertices)
  {
    throw svtkm::cont::ErrorBadValue("Not enough indices given to CellSetExplicit::AddCell.");
  }

  if (this->Data->NumberOfCellsAdded >= this->Data->CellPointIds.Shapes.GetNumberOfValues())
  {
    throw svtkm::cont::ErrorBadValue("Added more cells then expected.");
  }
  if (this->Data->ConnectivityAdded + numVertices >
      this->Data->CellPointIds.Connectivity.GetNumberOfValues())
  {
    throw svtkm::cont::ErrorBadValue(
      "Connectivity increased past estimated maximum connectivity.");
  }

  auto shapes = this->Data->CellPointIds.Shapes.GetPortalControl();
  auto conn = this->Data->CellPointIds.Connectivity.GetPortalControl();
  auto offsets = this->Data->CellPointIds.Offsets.GetPortalControl();

  shapes.Set(this->Data->NumberOfCellsAdded, cellType);
  for (svtkm::IdComponent iVec = 0; iVec < numVertices; ++iVec)
  {
    conn.Set(this->Data->ConnectivityAdded + iVec,
             Traits::GetComponent(ids, iVec));
  }

  this->Data->NumberOfCellsAdded++;
  this->Data->ConnectivityAdded += numVertices;

  // Set the end offset for the added cell:
  offsets.Set(this->Data->NumberOfCellsAdded, this->Data->ConnectivityAdded);
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>::CompleteAddingCells(svtkm::Id numPoints)
{
  this->Data->NumberOfPoints = numPoints;
  this->Data->CellPointIds.Connectivity.Shrink(this->Data->ConnectivityAdded);
  this->Data->CellPointIds.ElementsValid = true;

  if (this->Data->NumberOfCellsAdded != this->GetNumberOfCells())
  {
    throw svtkm::cont::ErrorBadValue("Did not add as many cells as expected.");
  }

  this->Data->NumberOfCellsAdded = -1;
  this->Data->ConnectivityAdded = -1;
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>
::Fill(svtkm::Id numPoints,
       const svtkm::cont::ArrayHandle<svtkm::UInt8, SST>& shapes,
       const svtkm::cont::ArrayHandle<svtkm::Id, CST>& connectivity,
       const svtkm::cont::ArrayHandle<svtkm::Id, OST>& offsets)
{
  // Validate inputs:
  // Even for an empty cellset, offsets must contain a single 0:
  SVTKM_ASSERT(offsets.GetNumberOfValues() > 0);
  // Must be [numCells + 1] offsets and [numCells] shapes
  SVTKM_ASSERT(offsets.GetNumberOfValues() == shapes.GetNumberOfValues() + 1);
  // The last offset must be the size of the connectivity array.
  SVTKM_ASSERT(svtkm::cont::ArrayGetValue(offsets.GetNumberOfValues() - 1,
                                        offsets) ==
              connectivity.GetNumberOfValues());

  this->Data->NumberOfPoints = numPoints;
  this->Data->CellPointIds.Shapes = shapes;
  this->Data->CellPointIds.Connectivity = connectivity;
  this->Data->CellPointIds.Offsets = offsets;

  this->Data->CellPointIds.ElementsValid = true;

  this->ResetConnectivity(TopologyElementTagPoint{}, TopologyElementTagCell{});
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
template <typename Device, typename VisitTopology, typename IncidentTopology>
SVTKM_CONT
auto CellSetExplicit<SST, CST, OST>
::PrepareForInput(Device, VisitTopology, IncidentTopology) const
-> typename ExecutionTypes<Device,
                           VisitTopology,
                           IncidentTopology>::ExecObjectType
{
  this->BuildConnectivity(Device{}, VisitTopology{}, IncidentTopology{});

  const auto& connectivity = this->GetConnectivity(VisitTopology{},
                                                   IncidentTopology{});
  SVTKM_ASSERT(connectivity.ElementsValid);

  using ExecObjType = typename ExecutionTypes<Device,
                                              VisitTopology,
                                              IncidentTopology>::ExecObjectType;

  return ExecObjType(connectivity.Shapes.PrepareForInput(Device{}),
                     connectivity.Connectivity.PrepareForInput(Device{}),
                     connectivity.Offsets.PrepareForInput(Device{}));
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
SVTKM_CONT auto CellSetExplicit<SST, CST, OST>
::GetShapesArray(VisitTopology, IncidentTopology) const
-> const typename ConnectivityChooser<VisitTopology,
                                      IncidentTopology>::ShapesArrayType&
{
  this->BuildConnectivity(svtkm::cont::DeviceAdapterTagAny{},
                          VisitTopology{},
                          IncidentTopology{});
  return this->GetConnectivity(VisitTopology{}, IncidentTopology{}).Shapes;
}

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
SVTKM_CONT
auto CellSetExplicit<SST, CST, OST>
::GetConnectivityArray(VisitTopology, IncidentTopology) const
-> const typename ConnectivityChooser<VisitTopology,
                                      IncidentTopology>::ConnectivityArrayType&
{
  this->BuildConnectivity(svtkm::cont::DeviceAdapterTagAny{},
                          VisitTopology{},
                          IncidentTopology{});
  return this->GetConnectivity(VisitTopology{},
                               IncidentTopology{}).Connectivity;
}

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
SVTKM_CONT
auto CellSetExplicit<SST, CST, OST>
::GetOffsetsArray(VisitTopology, IncidentTopology) const
-> const typename ConnectivityChooser<VisitTopology,
                                      IncidentTopology>::OffsetsArrayType&
{
  this->BuildConnectivity(svtkm::cont::DeviceAdapterTagAny{},
                          VisitTopology{},
                          IncidentTopology{});
  return this->GetConnectivity(VisitTopology{},
                               IncidentTopology{}).Offsets;
}

template <typename SST, typename CST, typename OST>
template <typename VisitTopology, typename IncidentTopology>
SVTKM_CONT
auto CellSetExplicit<SST, CST, OST>
::GetNumIndicesArray(VisitTopology visited, IncidentTopology incident) const
-> typename ConnectivityChooser<VisitTopology,
                                IncidentTopology>::NumIndicesArrayType
{
  auto offsets = this->GetOffsetsArray(visited, incident);
  const svtkm::Id numVals = offsets.GetNumberOfValues() - 1;
  return svtkm::cont::make_ArrayHandleDecorator(numVals,
                                               detail::NumIndicesDecorator{},
                                               std::move(offsets));
}

//----------------------------------------------------------------------------

template <typename SST, typename CST, typename OST>
SVTKM_CONT
std::shared_ptr<CellSet> CellSetExplicit<SST, CST, OST>::NewInstance() const
{
  return std::make_shared<CellSetExplicit>();
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>::DeepCopy(const CellSet* src)
{
  const auto* other = dynamic_cast<const CellSetExplicit*>(src);
  if (!other)
  {
    throw svtkm::cont::ErrorBadType("CellSetExplicit::DeepCopy types don't match");
  }

  ShapesArrayType shapes;
  ConnectivityArrayType conn;
  OffsetsArrayType offsets;

  const auto ct = svtkm::TopologyElementTagCell{};
  const auto pt = svtkm::TopologyElementTagPoint{};

  svtkm::cont::ArrayCopy(other->GetShapesArray(ct, pt), shapes);
  svtkm::cont::ArrayCopy(other->GetConnectivityArray(ct, pt), conn);
  svtkm::cont::ArrayCopy(other->GetOffsetsArray(ct, pt), offsets);

  this->Fill(other->GetNumberOfPoints(), shapes, conn, offsets);
}

//----------------------------------------------------------------------------

namespace detail
{

template <typename CellPointIdsT, typename PointCellIdsT>
struct BuildPointCellIdsFunctor
{
  BuildPointCellIdsFunctor(CellPointIdsT &cellPointIds,
                           PointCellIdsT &pointCellIds,
                           svtkm::Id numberOfPoints)
    : CellPointIds(cellPointIds)
    , PointCellIds(pointCellIds)
    , NumberOfPoints(numberOfPoints)
  {
  }

  template <typename Device>
  bool operator()(Device) const
  {
    internal::ComputeRConnTable(this->PointCellIds,
                                this->CellPointIds,
                                this->NumberOfPoints,
                                Device{});
    return true;
  }

  CellPointIdsT &CellPointIds;
  PointCellIdsT &PointCellIds;
  svtkm::Id NumberOfPoints;
};

} // detail

template <typename SST, typename CST, typename OST>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>
::BuildConnectivity(svtkm::cont::DeviceAdapterId,
                    svtkm::TopologyElementTagCell,
                    svtkm::TopologyElementTagPoint) const
{
  SVTKM_ASSERT(this->Data->CellPointIds.ElementsValid);
  // no-op
}

template <typename SST, typename CST, typename OST>
SVTKM_CONT
void CellSetExplicit<SST, CST, OST>
::BuildConnectivity(svtkm::cont::DeviceAdapterId device,
                    svtkm::TopologyElementTagPoint,
                    svtkm::TopologyElementTagCell) const
{
  if (!this->Data->PointCellIds.ElementsValid)
  {
    auto self = const_cast<Thisclass*>(this);
    using Func = detail::BuildPointCellIdsFunctor<CellPointIdsType, PointCellIdsType>;

    auto functor = Func(self->Data->CellPointIds,
                        self->Data->PointCellIds,
                        self->Data->NumberOfPoints);

    if (!svtkm::cont::TryExecuteOnDevice(device, functor))
    {
      throw svtkm::cont::ErrorExecution("Failed to run CellSetExplicit reverse "
                                       "connectivity builder.");
    }
  }
}
}
} // svtkm::cont

// clang-format on

#endif
