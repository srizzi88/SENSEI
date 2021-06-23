//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/internal/VirtualObjectTransfer.h>

#include <svtkm/cont/ErrorBadType.h>
#include <svtkm/cont/ErrorBadValue.h>

#include <array>
#include <memory>


namespace svtkm
{
namespace cont
{
namespace internal
{

SVTKM_CONT TransferInterface::~TransferInterface() = default;

bool TransferState::DeviceIdIsValid(svtkm::cont::DeviceAdapterId deviceId) const
{
  auto index = static_cast<std::size_t>(deviceId.GetValue());
  auto size = this->DeviceTransferState.size();

  if (!this->HostPointer)
  {
    throw svtkm::cont::ErrorBadValue(
      "No virtual object was bound before being asked to be executed");
  }
  if (index >= size)
  {
    std::string msg = "An invalid DeviceAdapter[id=" + std::to_string(deviceId.GetValue()) +
      ", name=" + deviceId.GetName() + "] was used when trying to construct a virtual object.";
    throw svtkm::cont::ErrorBadType(msg);
  }

  if (!this->DeviceTransferState[index])
  {
    const std::string msg =
      "SVTK-m was asked to transfer a VirtualObjectHandle for execution on DeviceAdapter[id=" +
      std::to_string(deviceId.GetValue()) + ", name=" + deviceId.GetName() +
      "]. It can't as this VirtualObjectHandle was not constructed/bound with this "
      "DeviceAdapter in the list of valid DeviceAdapters.";
    throw svtkm::cont::ErrorBadType(msg);
  }

  return true;
}
}
}
}
