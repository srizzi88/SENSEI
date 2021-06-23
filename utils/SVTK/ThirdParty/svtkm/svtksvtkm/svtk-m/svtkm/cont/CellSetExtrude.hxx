//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_CellSetExtrude_hxx
#define svtk_m_cont_CellSetExtrude_hxx

namespace
{
struct ComputeReverseMapping : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn cellIndex, WholeArrayOut cellIds);

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  SVTKM_EXEC void operator()(svtkm::Id cellId, PortalType&& pointIdValue) const
  {
    //3 as we are building the connectivity for triangles
    const svtkm::Id offset = 3 * cellId;
    pointIdValue.Set(offset, static_cast<svtkm::Int32>(cellId));
    pointIdValue.Set(offset + 1, static_cast<svtkm::Int32>(cellId));
    pointIdValue.Set(offset + 2, static_cast<svtkm::Int32>(cellId));
  }
};

struct ComputePrevNode : public svtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn nextNode, WholeArrayOut prevNodeArray);
  typedef void ExecutionSignature(InputIndex, _1, _2);

  template <typename PortalType>
  SVTKM_EXEC void operator()(svtkm::Id idx, svtkm::Int32 next, PortalType& prevs) const
  {
    prevs.Set(static_cast<svtkm::Id>(next), static_cast<svtkm::Int32>(idx));
  }
};

} // anonymous namespace

namespace svtkm
{
namespace cont
{
template <typename Device>
SVTKM_CONT void CellSetExtrude::BuildReverseConnectivity(Device)
{
  svtkm::cont::Invoker invoke(Device{});

  // create a mapping of where each key is the point id and the value
  // is the cell id. We
  const svtkm::Id numberOfPointsPerCell = 3;
  const svtkm::Id rconnSize = this->NumberOfCellsPerPlane * numberOfPointsPerCell;

  svtkm::cont::ArrayHandle<svtkm::Int32> pointIdKey;
  svtkm::cont::DeviceAdapterAlgorithm<Device>::Copy(this->Connectivity, pointIdKey);

  this->RConnectivity.Allocate(rconnSize);
  invoke(ComputeReverseMapping{},
         svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(0, 1, this->NumberOfCellsPerPlane),
         this->RConnectivity);

  svtkm::cont::DeviceAdapterAlgorithm<Device>::SortByKey(pointIdKey, this->RConnectivity);

  // now we can compute the counts and offsets
  svtkm::cont::ArrayHandle<svtkm::Int32> reducedKeys;
  svtkm::cont::DeviceAdapterAlgorithm<Device>::ReduceByKey(
    pointIdKey,
    svtkm::cont::make_ArrayHandleConstant(svtkm::Int32(1), static_cast<svtkm::Int32>(rconnSize)),
    reducedKeys,
    this->RCounts,
    svtkm::Add{});

  svtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(this->RCounts, this->ROffsets);

  // compute PrevNode from NextNode
  this->PrevNode.Allocate(this->NextNode.GetNumberOfValues());
  invoke(ComputePrevNode{}, this->NextNode, this->PrevNode);

  this->ReverseConnectivityBuilt = true;
}
template <typename Device>
CellSetExtrude::ConnectivityP2C<Device> CellSetExtrude::PrepareForInput(
  Device,
  svtkm::TopologyElementTagCell,
  svtkm::TopologyElementTagPoint) const
{
  return ConnectivityP2C<Device>(this->Connectivity.PrepareForInput(Device{}),
                                 this->NextNode.PrepareForInput(Device{}),
                                 this->NumberOfCellsPerPlane,
                                 this->NumberOfPointsPerPlane,
                                 this->NumberOfPlanes,
                                 this->IsPeriodic);
}


template <typename Device>
SVTKM_CONT CellSetExtrude::ConnectivityC2P<Device> CellSetExtrude::PrepareForInput(
  Device,
  svtkm::TopologyElementTagPoint,
  svtkm::TopologyElementTagCell) const
{
  if (!this->ReverseConnectivityBuilt)
  {
    const_cast<CellSetExtrude*>(this)->BuildReverseConnectivity(Device{});
  }
  return ConnectivityC2P<Device>(this->RConnectivity.PrepareForInput(Device{}),
                                 this->ROffsets.PrepareForInput(Device{}),
                                 this->RCounts.PrepareForInput(Device{}),
                                 this->PrevNode.PrepareForInput(Device{}),
                                 this->NumberOfCellsPerPlane,
                                 this->NumberOfPointsPerPlane,
                                 this->NumberOfPlanes);
}
}
} // svtkm::cont
#endif
