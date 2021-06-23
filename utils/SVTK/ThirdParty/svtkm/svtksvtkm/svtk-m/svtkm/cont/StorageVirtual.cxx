//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#define svtk_m_cont_StorageVirtual_cxx
#include <svtkm/cont/StorageVirtual.h>

namespace svtkm
{
namespace cont
{
namespace internal
{
namespace detail
{


//--------------------------------------------------------------------
StorageVirtual::StorageVirtual(const StorageVirtual& src)
  : HostUpToDate(src.HostUpToDate)
  , DeviceUpToDate(src.DeviceUpToDate)
  , DeviceTransferState(src.DeviceTransferState)
{
}

//--------------------------------------------------------------------
StorageVirtual::StorageVirtual(StorageVirtual&& src) noexcept
  : HostUpToDate(src.HostUpToDate),
    DeviceUpToDate(src.DeviceUpToDate),
    DeviceTransferState(std::move(src.DeviceTransferState))
{
}

//--------------------------------------------------------------------
StorageVirtual& StorageVirtual::operator=(const StorageVirtual& src)
{
  this->HostUpToDate = src.HostUpToDate;
  this->DeviceUpToDate = src.DeviceUpToDate;
  this->DeviceTransferState = src.DeviceTransferState;
  return *this;
}

//--------------------------------------------------------------------
StorageVirtual& StorageVirtual::operator=(StorageVirtual&& src) noexcept
{
  this->HostUpToDate = src.HostUpToDate;
  this->DeviceUpToDate = src.DeviceUpToDate;
  this->DeviceTransferState = std::move(src.DeviceTransferState);
  return *this;
}

//--------------------------------------------------------------------
StorageVirtual::~StorageVirtual()
{
}

//--------------------------------------------------------------------
void StorageVirtual::DropExecutionPortal()
{
  this->DeviceTransferState->releaseDevice();
  this->DeviceUpToDate = false;
}

//--------------------------------------------------------------------
void StorageVirtual::DropAllPortals()
{
  this->DeviceTransferState->releaseAll();
  this->HostUpToDate = false;
  this->DeviceUpToDate = false;
}

//--------------------------------------------------------------------
std::unique_ptr<StorageVirtual> StorageVirtual::NewInstance() const
{
  return this->MakeNewInstance();
}

//--------------------------------------------------------------------
const svtkm::internal::PortalVirtualBase* StorageVirtual::PrepareForInput(
  svtkm::cont::DeviceAdapterId devId) const
{
  if (devId == svtkm::cont::DeviceAdapterTagUndefined())
  {
    throw svtkm::cont::ErrorBadValue("device should not be SVTKM_DEVICE_ADAPTER_UNDEFINED");
  }

  const bool needsUpload = !(this->DeviceTransferState->valid(devId) && this->DeviceUpToDate);

  if (needsUpload)
  { //Either transfer state is pointing to another device, or has
    //had the execution resources released. Either way we
    //need to re-transfer the execution information
    auto* payload = this->DeviceTransferState.get();
    this->TransferPortalForInput(*payload, devId);
    this->DeviceUpToDate = true;
  }
  return this->DeviceTransferState->devicePtr();
}

//--------------------------------------------------------------------
const svtkm::internal::PortalVirtualBase* StorageVirtual::PrepareForOutput(
  svtkm::Id numberOfValues,
  svtkm::cont::DeviceAdapterId devId)
{
  if (devId == svtkm::cont::DeviceAdapterTagUndefined())
  {
    throw svtkm::cont::ErrorBadValue("device should not be SVTKM_DEVICE_ADAPTER_UNDEFINED");
  }

  const bool needsUpload = !(this->DeviceTransferState->valid(devId) && this->DeviceUpToDate);
  if (needsUpload)
  {
    this->TransferPortalForOutput(
      *(this->DeviceTransferState), OutputMode::WRITE, numberOfValues, devId);
    this->HostUpToDate = false;
    this->DeviceUpToDate = true;
  }
  return this->DeviceTransferState->devicePtr();
}

//--------------------------------------------------------------------
const svtkm::internal::PortalVirtualBase* StorageVirtual::PrepareForInPlace(
  svtkm::cont::DeviceAdapterId devId)
{
  if (devId == svtkm::cont::DeviceAdapterTagUndefined())
  {
    throw svtkm::cont::ErrorBadValue("device should not be SVTKM_DEVICE_ADAPTER_UNDEFINED");
  }

  const bool needsUpload = !(this->DeviceTransferState->valid(devId) && this->DeviceUpToDate);
  if (needsUpload)
  {
    svtkm::Id numberOfValues = this->GetNumberOfValues();
    this->TransferPortalForOutput(
      *(this->DeviceTransferState), OutputMode::READ_WRITE, numberOfValues, devId);
    this->HostUpToDate = false;
    this->DeviceUpToDate = true;
  }
  return this->DeviceTransferState->devicePtr();
}

//--------------------------------------------------------------------
const svtkm::internal::PortalVirtualBase* StorageVirtual::GetPortalControl()
{
  if (!this->HostUpToDate)
  {
    //we need to prepare for input and grab the host ptr
    auto* payload = this->DeviceTransferState.get();
    this->ControlPortalForOutput(*payload);
  }

  this->DeviceUpToDate = false;
  this->HostUpToDate = true;
  return this->DeviceTransferState->hostPtr();
}

//--------------------------------------------------------------------
const svtkm::internal::PortalVirtualBase* StorageVirtual::GetPortalConstControl() const
{
  if (!this->HostUpToDate)
  {
    //we need to prepare for input and grab the host ptr
    svtkm::cont::internal::TransferInfoArray* payload = this->DeviceTransferState.get();
    this->ControlPortalForInput(*payload);
  }
  this->HostUpToDate = true;
  return this->DeviceTransferState->hostPtr();
}

//--------------------------------------------------------------------
DeviceAdapterId StorageVirtual::GetDeviceAdapterId() const noexcept
{
  return this->DeviceTransferState->deviceId();
}

//--------------------------------------------------------------------
void StorageVirtual::ControlPortalForOutput(svtkm::cont::internal::TransferInfoArray&)
{
  throw svtkm::cont::ErrorBadValue(
    "StorageTagVirtual by default doesn't support control side writes.");
}

//--------------------------------------------------------------------
void StorageVirtual::TransferPortalForOutput(svtkm::cont::internal::TransferInfoArray&,
                                             OutputMode,
                                             svtkm::Id,
                                             svtkm::cont::DeviceAdapterId)
{
  throw svtkm::cont::ErrorBadValue("StorageTagVirtual by default doesn't support exec side writes.");
}

#define SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(T)                                                \
  template class SVTKM_CONT_EXPORT ArrayTransferVirtual<T>;                                         \
  template class SVTKM_CONT_EXPORT ArrayTransferVirtual<svtkm::Vec<T, 2>>;                           \
  template class SVTKM_CONT_EXPORT ArrayTransferVirtual<svtkm::Vec<T, 3>>;                           \
  template class SVTKM_CONT_EXPORT ArrayTransferVirtual<svtkm::Vec<T, 4>>

SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(char);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(svtkm::Int8);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(svtkm::UInt8);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(svtkm::Int16);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(svtkm::UInt16);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(svtkm::Int32);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(svtkm::UInt32);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(svtkm::Int64);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(svtkm::UInt64);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(svtkm::Float32);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE(svtkm::Float64);

#undef SVTK_M_ARRAY_TRANSFER_VIRTUAL_INSTANTIATE

#define SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(T)                                                       \
  template class SVTKM_CONT_EXPORT StorageVirtualImpl<T, SVTKM_DEFAULT_STORAGE_TAG>;                 \
  template class SVTKM_CONT_EXPORT StorageVirtualImpl<svtkm::Vec<T, 2>, SVTKM_DEFAULT_STORAGE_TAG>;   \
  template class SVTKM_CONT_EXPORT StorageVirtualImpl<svtkm::Vec<T, 3>, SVTKM_DEFAULT_STORAGE_TAG>;   \
  template class SVTKM_CONT_EXPORT StorageVirtualImpl<svtkm::Vec<T, 4>, SVTKM_DEFAULT_STORAGE_TAG>

SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(char);
SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(svtkm::Int8);
SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(svtkm::UInt8);
SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(svtkm::Int16);
SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(svtkm::UInt16);
SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(svtkm::Int32);
SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(svtkm::UInt32);
SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(svtkm::Int64);
SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(svtkm::UInt64);
SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(svtkm::Float32);
SVTK_M_STORAGE_VIRTUAL_INSTANTIATE(svtkm::Float64);

#undef SVTK_M_STORAGE_VIRTUAL_INSTANTIATE
}
}
}
} // namespace svtkm::cont::internal::detail
