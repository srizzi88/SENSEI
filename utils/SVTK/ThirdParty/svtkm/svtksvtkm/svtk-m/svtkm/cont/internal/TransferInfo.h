//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_TransferInfo_h
#define svtk_m_cont_internal_TransferInfo_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/Types.h>
#include <svtkm/cont/DeviceAdapterTag.h>
#include <svtkm/internal/ArrayPortalVirtual.h>

#include <memory>

namespace svtkm
{

namespace internal
{
class PortalVirtualBase;
}

namespace cont
{
namespace internal
{

struct SVTKM_CONT_EXPORT TransferInfoArray
{
  bool valid(svtkm::cont::DeviceAdapterId tagValue) const noexcept;

  void updateHost(std::unique_ptr<svtkm::internal::PortalVirtualBase>&& host) noexcept;
  void updateDevice(
    svtkm::cont::DeviceAdapterId id,
    std::unique_ptr<svtkm::internal::PortalVirtualBase>&& host_copy, //NOT the same as host version
    const svtkm::internal::PortalVirtualBase* device,
    const std::shared_ptr<void>& state) noexcept;
  void releaseDevice();
  void releaseAll();

  const svtkm::internal::PortalVirtualBase* hostPtr() const noexcept { return this->Host.get(); }
  const svtkm::internal::PortalVirtualBase* devicePtr() const noexcept { return this->Device; }
  svtkm::cont::DeviceAdapterId deviceId() const noexcept { return this->DeviceId; }

  std::shared_ptr<void>& state() noexcept { return this->DeviceTransferState; }

private:
  svtkm::cont::DeviceAdapterId DeviceId = svtkm::cont::DeviceAdapterTagUndefined{};
  std::unique_ptr<svtkm::internal::PortalVirtualBase> Host = nullptr;
  std::unique_ptr<svtkm::internal::PortalVirtualBase> HostCopyOfDevice = nullptr;
  const svtkm::internal::PortalVirtualBase* Device = nullptr;
  std::shared_ptr<void> DeviceTransferState = nullptr;
};
}
}
}

#endif
