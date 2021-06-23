//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_StorageVirtual_hxx
#define svtk_m_cont_StorageVirtual_hxx

#include <svtkm/cont/StorageVirtual.h>
#include <svtkm/cont/TryExecute.h>
#include <svtkm/cont/internal/TransferInfo.h>

#include <svtkm/cont/internal/VirtualObjectTransfer.h>
#include <svtkm/cont/internal/VirtualObjectTransferShareWithControl.h>

namespace svtkm
{
namespace cont
{
namespace detail
{
template <typename DerivedPortal>
struct TransferToDevice
{
  template <typename DeviceAdapterTag, typename Payload, typename... Args>
  inline bool operator()(DeviceAdapterTag devId, Payload&& payload, Args&&... args) const
  {
    using TransferType = cont::internal::VirtualObjectTransfer<DerivedPortal, DeviceAdapterTag>;
    using shared_memory_transfer =
      std::is_base_of<svtkm::cont::internal::VirtualObjectTransferShareWithControl<DerivedPortal>,
                      TransferType>;

    return this->Transfer(
      devId, shared_memory_transfer{}, std::forward<Payload>(payload), std::forward<Args>(args)...);
  }

  template <typename DeviceAdapterTag, typename Payload, typename... Args>
  inline bool Transfer(DeviceAdapterTag devId,
                       std::true_type,
                       Payload&& payload,
                       Args&&... args) const
  { //shared memory transfer so we just need
    auto smp_ptr = new DerivedPortal(std::forward<Args>(args)...);
    auto host = std::unique_ptr<DerivedPortal>(smp_ptr);
    payload.updateDevice(devId, std::move(host), smp_ptr, nullptr);

    return true;
  }

  template <typename DeviceAdapterTag, typename Payload, typename... Args>
  inline bool Transfer(DeviceAdapterTag devId,
                       std::false_type,
                       Payload&& payload,
                       Args&&... args) const
  { //separate memory transfer
    //construct all new transfer payload
    using TransferType = cont::internal::VirtualObjectTransfer<DerivedPortal, DeviceAdapterTag>;

    auto host = std::unique_ptr<DerivedPortal>(new DerivedPortal(std::forward<Args>(args)...));
    auto transfer = std::make_shared<TransferType>(host.get());
    auto device = transfer->PrepareForExecution(true);

    payload.updateDevice(devId, std::move(host), device, std::static_pointer_cast<void>(transfer));

    return true;
  }
};
} // namespace detail

template <typename DerivedPortal, typename... Args>
inline void make_transferToDevice(svtkm::cont::DeviceAdapterId devId, Args&&... args)
{
  svtkm::cont::TryExecuteOnDevice(
    devId, detail::TransferToDevice<DerivedPortal>{}, std::forward<Args>(args)...);
}

template <typename DerivedPortal, typename Payload, typename... Args>
inline void make_hostPortal(Payload&& payload, Args&&... args)
{
  auto host = std::unique_ptr<DerivedPortal>(new DerivedPortal(std::forward<Args>(args)...));
  payload.updateHost(std::move(host));
}

namespace internal
{
namespace detail
{

SVTKM_CONT
template <typename T, typename S>
StorageVirtualImpl<T, S>::StorageVirtualImpl(const svtkm::cont::ArrayHandle<T, S>& ah)
  : svtkm::cont::internal::detail::StorageVirtual()
  , Handle(ah)
{
}

SVTKM_CONT
template <typename T, typename S>
StorageVirtualImpl<T, S>::StorageVirtualImpl(svtkm::cont::ArrayHandle<T, S>&& ah) noexcept
  : svtkm::cont::internal::detail::StorageVirtual(),
    Handle(std::move(ah))
{
}

/// release execution side resources
template <typename T, typename S>
void StorageVirtualImpl<T, S>::ReleaseResourcesExecution()
{
  this->DropExecutionPortal();
  this->Handle.ReleaseResourcesExecution();
}

/// release control side resources
template <typename T, typename S>
void StorageVirtualImpl<T, S>::ReleaseResources()
{
  this->DropAllPortals();
  this->Handle.ReleaseResources();
}

template <typename T, typename S>
void StorageVirtualImpl<T, S>::Allocate(svtkm::Id numberOfValues)
{
  this->DropAllPortals();
  this->Handle.Allocate(numberOfValues);
}

template <typename T, typename S>
void StorageVirtualImpl<T, S>::Shrink(svtkm::Id numberOfValues)
{
  this->DropAllPortals();
  this->Handle.Shrink(numberOfValues);
}

struct PortalWrapperToDevice
{
  template <typename DeviceAdapterTag, typename Handle>
  inline bool operator()(DeviceAdapterTag device,
                         Handle&& handle,
                         svtkm::cont::internal::TransferInfoArray& payload) const
  {
    auto portal = handle.PrepareForInput(device);
    using DerivedPortal = svtkm::ArrayPortalWrapper<decltype(portal)>;
    svtkm::cont::detail::TransferToDevice<DerivedPortal> transfer;
    return transfer(device, payload, portal);
  }
  template <typename DeviceAdapterTag, typename Handle>
  inline bool operator()(DeviceAdapterTag device,
                         Handle&& handle,
                         svtkm::Id numberOfValues,
                         svtkm::cont::internal::TransferInfoArray& payload,
                         svtkm::cont::internal::detail::StorageVirtual::OutputMode mode) const
  {
    using ACCESS_MODE = svtkm::cont::internal::detail::StorageVirtual::OutputMode;
    if (mode == ACCESS_MODE::WRITE)
    {
      auto portal = handle.PrepareForOutput(numberOfValues, device);
      using DerivedPortal = svtkm::ArrayPortalWrapper<decltype(portal)>;
      svtkm::cont::detail::TransferToDevice<DerivedPortal> transfer;
      return transfer(device, payload, portal);
    }
    else
    {
      auto portal = handle.PrepareForInPlace(device);
      using DerivedPortal = svtkm::ArrayPortalWrapper<decltype(portal)>;
      svtkm::cont::detail::TransferToDevice<DerivedPortal> transfer;
      return transfer(device, payload, portal);
    }
  }
};

template <typename T, typename S>
void StorageVirtualImpl<T, S>::ControlPortalForInput(
  svtkm::cont::internal::TransferInfoArray& payload) const
{
  auto portal = this->Handle.GetPortalConstControl();
  using DerivedPortal = svtkm::ArrayPortalWrapper<decltype(portal)>;
  svtkm::cont::make_hostPortal<DerivedPortal>(payload, portal);
}

template <typename HandleType>
inline void make_writableHostPortal(std::true_type,
                                    svtkm::cont::internal::TransferInfoArray& payload,
                                    HandleType& handle)
{
  auto portal = handle.GetPortalControl();
  using DerivedPortal = svtkm::ArrayPortalWrapper<decltype(portal)>;
  svtkm::cont::make_hostPortal<DerivedPortal>(payload, portal);
}
template <typename HandleType>
inline void make_writableHostPortal(std::false_type,
                                    svtkm::cont::internal::TransferInfoArray& payload,
                                    HandleType&)
{
  payload.updateHost(nullptr);
  throw svtkm::cont::ErrorBadValue(
    "ArrayHandleAny was bound to an ArrayHandle that doesn't support output.");
}

template <typename T, typename S>
void StorageVirtualImpl<T, S>::ControlPortalForOutput(
  svtkm::cont::internal::TransferInfoArray& payload)
{
  using HT = svtkm::cont::ArrayHandle<T, S>;
  constexpr auto isWritable = typename svtkm::cont::internal::IsWritableArrayHandle<HT>::type{};

  detail::make_writableHostPortal(isWritable, payload, this->Handle);
}

template <typename T, typename S>
void StorageVirtualImpl<T, S>::TransferPortalForInput(
  svtkm::cont::internal::TransferInfoArray& payload,
  svtkm::cont::DeviceAdapterId devId) const
{
  svtkm::cont::TryExecuteOnDevice(devId, detail::PortalWrapperToDevice(), this->Handle, payload);
}


template <typename T, typename S>
void StorageVirtualImpl<T, S>::TransferPortalForOutput(
  svtkm::cont::internal::TransferInfoArray& payload,
  svtkm::cont::internal::detail::StorageVirtual::OutputMode mode,
  svtkm::Id numberOfValues,
  svtkm::cont::DeviceAdapterId devId)
{
  svtkm::cont::TryExecuteOnDevice(
    devId, detail::PortalWrapperToDevice(), this->Handle, numberOfValues, payload, mode);
}
} // namespace detail

template <typename T>
void Storage<T, svtkm::cont::StorageTagVirtual>::Allocate(svtkm::Id numberOfValues)
{
  if (this->VirtualStorage)
  {
    this->VirtualStorage->Allocate(numberOfValues);
  }
  else if (numberOfValues != 0)
  {
    throw svtkm::cont::ErrorBadAllocation("Attempted to allocate memory in a virtual array that "
                                         "does not have an underlying concrete array.");
  }
  else
  {
    // Allocating a non-existing array to 0 is OK.
  }
}

template <typename T>
void Storage<T, svtkm::cont::StorageTagVirtual>::Shrink(svtkm::Id numberOfValues)
{
  if (this->VirtualStorage)
  {
    this->VirtualStorage->Shrink(numberOfValues);
  }
  else if (numberOfValues != 0)
  {
    throw svtkm::cont::ErrorBadAllocation(
      "Attempted to shrink a virtual array that does not have an underlying concrete array.");
  }
  else
  {
    // Shrinking a non-existing array to 0 is OK.
  }
}

template <typename T>
void Storage<T, svtkm::cont::StorageTagVirtual>::ReleaseResources()
{
  if (this->VirtualStorage)
  {
    this->VirtualStorage->ReleaseResources();
  }
  else
  {
    // No concrete array, nothing allocated, nothing to do.
  }
}

template <typename T>
Storage<T, svtkm::cont::StorageTagVirtual> Storage<T, svtkm::cont::StorageTagVirtual>::NewInstance()
  const
{
  if (this->GetStorageVirtual())
  {
    return Storage<T, svtkm::cont::StorageTagVirtual>(this->GetStorageVirtual()->NewInstance());
  }
  else
  {
    return Storage<T, svtkm::cont::StorageTagVirtual>();
  }
}

namespace detail
{

template <typename T>
class SVTKM_ALWAYS_EXPORT ArrayTransferVirtual
{
  using StorageType = svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagVirtual>;
  svtkm::cont::internal::detail::StorageVirtual* Storage;

public:
  using ValueType = T;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = svtkm::ArrayPortalRef<ValueType>;
  using PortalConstExecution = svtkm::ArrayPortalRef<ValueType>;

  SVTKM_CONT ArrayTransferVirtual(StorageType* storage)
    : Storage(storage->GetStorageVirtual())
  {
  }

  SVTKM_CONT svtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  SVTKM_CONT PortalConstExecution PrepareForInput(svtkm::cont::DeviceAdapterId device)
  {
    return svtkm::make_ArrayPortalRef(static_cast<const svtkm::ArrayPortalVirtual<ValueType>*>(
                                       this->Storage->PrepareForInput(device)),
                                     this->GetNumberOfValues());
  }

  SVTKM_CONT PortalExecution PrepareForOutput(svtkm::Id numberOfValues,
                                             svtkm::cont::DeviceAdapterId device)
  {
    return make_ArrayPortalRef(static_cast<const svtkm::ArrayPortalVirtual<T>*>(
                                 this->Storage->PrepareForOutput(numberOfValues, device)),
                               numberOfValues);
  }

  SVTKM_CONT PortalExecution PrepareForInPlace(svtkm::cont::DeviceAdapterId device)
  {
    return svtkm::make_ArrayPortalRef(static_cast<const svtkm::ArrayPortalVirtual<ValueType>*>(
                                       this->Storage->PrepareForInPlace(device)),
                                     this->GetNumberOfValues());
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal array handles should
    // automatically retrieve the output data as necessary.
  }

  SVTKM_CONT void Shrink(svtkm::Id numberOfValues) { this->Storage->Shrink(numberOfValues); }

  SVTKM_CONT void ReleaseResources() { this->Storage->ReleaseResourcesExecution(); }
};

#ifndef svtk_m_cont_StorageVirtual_cxx

#define SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(T)                                                     \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayTransferVirtual<T>;                         \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayTransferVirtual<svtkm::Vec<T, 2>>;           \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayTransferVirtual<svtkm::Vec<T, 3>>;           \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayTransferVirtual<svtkm::Vec<T, 4>>

SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(char);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(svtkm::Int8);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(svtkm::UInt8);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(svtkm::Int16);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(svtkm::UInt16);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(svtkm::Int32);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(svtkm::UInt32);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(svtkm::Int64);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(svtkm::UInt64);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(svtkm::Float32);
SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT(svtkm::Float64);

#undef SVTK_M_ARRAY_TRANSFER_VIRTUAL_EXPORT

#define SVTK_M_STORAGE_VIRTUAL_EXPORT(T)                                                            \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT StorageVirtualImpl<T, SVTKM_DEFAULT_STORAGE_TAG>; \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT                                                  \
    StorageVirtualImpl<svtkm::Vec<T, 2>, SVTKM_DEFAULT_STORAGE_TAG>;                                 \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT                                                  \
    StorageVirtualImpl<svtkm::Vec<T, 3>, SVTKM_DEFAULT_STORAGE_TAG>;                                 \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT                                                  \
    StorageVirtualImpl<svtkm::Vec<T, 4>, SVTKM_DEFAULT_STORAGE_TAG>

SVTK_M_STORAGE_VIRTUAL_EXPORT(char);
SVTK_M_STORAGE_VIRTUAL_EXPORT(svtkm::Int8);
SVTK_M_STORAGE_VIRTUAL_EXPORT(svtkm::UInt8);
SVTK_M_STORAGE_VIRTUAL_EXPORT(svtkm::Int16);
SVTK_M_STORAGE_VIRTUAL_EXPORT(svtkm::UInt16);
SVTK_M_STORAGE_VIRTUAL_EXPORT(svtkm::Int32);
SVTK_M_STORAGE_VIRTUAL_EXPORT(svtkm::UInt32);
SVTK_M_STORAGE_VIRTUAL_EXPORT(svtkm::Int64);
SVTK_M_STORAGE_VIRTUAL_EXPORT(svtkm::UInt64);
SVTK_M_STORAGE_VIRTUAL_EXPORT(svtkm::Float32);
SVTK_M_STORAGE_VIRTUAL_EXPORT(svtkm::Float64);

#undef SVTK_M_STORAGE_VIRTUAL_EXPORT

#endif //!svtk_m_cont_StorageVirtual_cxx
} // namespace detail

template <typename T, typename Device>
struct ArrayTransfer<T, svtkm::cont::StorageTagVirtual, Device> : detail::ArrayTransferVirtual<T>
{
  using Superclass = detail::ArrayTransferVirtual<T>;

  SVTKM_CONT ArrayTransfer(svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagVirtual>* storage)
    : Superclass(storage)
  {
  }

  SVTKM_CONT typename Superclass::PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return this->Superclass::PrepareForInput(Device());
  }

  SVTKM_CONT typename Superclass::PortalExecution PrepareForOutput(svtkm::Id numberOfValues)
  {
    return this->Superclass::PrepareForOutput(numberOfValues, Device());
  }

  SVTKM_CONT typename Superclass::PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    return this->Superclass::PrepareForInPlace(Device());
  }
};
}
}
} // namespace svtkm::cont::internal

#endif // svtk_m_cont_StorageVirtual_hxx
