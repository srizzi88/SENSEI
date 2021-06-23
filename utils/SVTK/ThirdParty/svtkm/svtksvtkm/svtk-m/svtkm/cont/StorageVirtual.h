//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_StorageVirtual_h
#define svtk_m_cont_StorageVirtual_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ErrorBadType.h>
#include <svtkm/cont/Storage.h>
#include <svtkm/cont/internal/TransferInfo.h>
#include <svtkm/internal/ArrayPortalVirtual.h>

#include <typeinfo>

namespace svtkm
{
namespace cont
{

struct SVTKM_ALWAYS_EXPORT StorageTagVirtual
{
};

namespace internal
{

namespace detail
{

class SVTKM_CONT_EXPORT StorageVirtual
{
public:
  /// \brief construct storage that SVTK-m is responsible for
  StorageVirtual() = default;
  StorageVirtual(const StorageVirtual& src);
  StorageVirtual(StorageVirtual&& src) noexcept;
  StorageVirtual& operator=(const StorageVirtual& src);
  StorageVirtual& operator=(StorageVirtual&& src) noexcept;

  virtual ~StorageVirtual();

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  /// Only needs to overridden by subclasses such as Zip that have member
  /// variables that themselves have execution memory
  virtual void ReleaseResourcesExecution() = 0;

  /// Releases all resources in both the control and execution environments.
  ///
  /// Only needs to overridden by subclasses such as Zip that have member
  /// variables that themselves have execution memory
  virtual void ReleaseResources() = 0;

  /// Returns the number of entries in the array.
  ///
  virtual svtkm::Id GetNumberOfValues() const = 0;

  /// \brief Allocates an array large enough to hold the given number of values.
  ///
  /// The allocation may be done on an already existing array, but can wipe out
  /// any data already in the array. This method can throw
  /// ErrorBadAllocation if the array cannot be allocated or
  /// ErrorBadValue if the allocation is not feasible (for example, the
  /// array storage is read-only).
  ///
  virtual void Allocate(svtkm::Id numberOfValues) = 0;

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  virtual void Shrink(svtkm::Id numberOfValues) = 0;

  /// Determines if storage types matches the type passed in.
  ///
  template <typename DerivedStorage>
  bool IsType() const
  { //needs optimizations based on platform. !OSX can use typeid
    return nullptr != dynamic_cast<const DerivedStorage*>(this);
  }

  /// \brief Create a new storage of the same type as this storage.
  ///
  /// This method creates a new storage that is the same type as this one and
  /// returns a unique_ptr for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  std::unique_ptr<StorageVirtual> NewInstance() const;

  template <typename DerivedStorage>
  const DerivedStorage* Cast() const
  {
    const DerivedStorage* derived = dynamic_cast<const DerivedStorage*>(this);
    if (!derived)
    {
      SVTKM_LOG_CAST_FAIL(*this, DerivedStorage);
      throwFailedDynamicCast("StorageVirtual", svtkm::cont::TypeToString<DerivedStorage>());
    }
    SVTKM_LOG_CAST_SUCC(*this, derived);
    return derived;
  }

  const svtkm::internal::PortalVirtualBase* PrepareForInput(svtkm::cont::DeviceAdapterId devId) const;

  const svtkm::internal::PortalVirtualBase* PrepareForOutput(svtkm::Id numberOfValues,
                                                            svtkm::cont::DeviceAdapterId devId);

  const svtkm::internal::PortalVirtualBase* PrepareForInPlace(svtkm::cont::DeviceAdapterId devId);

  //This needs to cause a host side sync!
  //This needs to work before we execute on a device
  const svtkm::internal::PortalVirtualBase* GetPortalControl();

  //This needs to cause a host side sync!
  //This needs to work before we execute on a device
  const svtkm::internal::PortalVirtualBase* GetPortalConstControl() const;

  /// Returns the DeviceAdapterId for the current device. If there is no device
  /// with an up-to-date copy of the data, SVTKM_DEVICE_ADAPTER_UNDEFINED is
  /// returned.
  DeviceAdapterId GetDeviceAdapterId() const noexcept;


  enum struct OutputMode
  {
    WRITE,
    READ_WRITE
  };

protected:
  /// Drop the reference to the execution portal. The underlying array handle might still be
  /// managing data on the execution side, but our references might be out of date, so drop
  /// them and refresh them later if necessary.
  void DropExecutionPortal();

  /// Drop the reference to all portals. The underlying array handle might still be managing data,
  /// but our references might be out of date, so drop them and refresh them later if necessary.
  void DropAllPortals();

private:
  //Memory management routines
  // virtual void DoAllocate(svtkm::Id numberOfValues) = 0;
  // virtual void DoShrink(svtkm::Id numberOfValues) = 0;

  //RTTI routines
  virtual std::unique_ptr<StorageVirtual> MakeNewInstance() const = 0;

  //Portal routines
  virtual void ControlPortalForInput(svtkm::cont::internal::TransferInfoArray& payload) const = 0;
  virtual void ControlPortalForOutput(svtkm::cont::internal::TransferInfoArray& payload);


  virtual void TransferPortalForInput(svtkm::cont::internal::TransferInfoArray& payload,
                                      svtkm::cont::DeviceAdapterId devId) const = 0;
  virtual void TransferPortalForOutput(svtkm::cont::internal::TransferInfoArray& payload,
                                       OutputMode mode,
                                       svtkm::Id numberOfValues,
                                       svtkm::cont::DeviceAdapterId devId);

  //These might need to exist in TransferInfoArray
  mutable bool HostUpToDate = false;
  mutable bool DeviceUpToDate = false;
  std::shared_ptr<svtkm::cont::internal::TransferInfoArray> DeviceTransferState =
    std::make_shared<svtkm::cont::internal::TransferInfoArray>();
};

template <typename T, typename S>
class SVTKM_ALWAYS_EXPORT StorageVirtualImpl final
  : public svtkm::cont::internal::detail::StorageVirtual
{
public:
  SVTKM_CONT
  explicit StorageVirtualImpl(const svtkm::cont::ArrayHandle<T, S>& ah);

  explicit StorageVirtualImpl(svtkm::cont::ArrayHandle<T, S>&& ah) noexcept;

  SVTKM_CONT
  ~StorageVirtualImpl() override = default;

  const svtkm::cont::ArrayHandle<T, S>& GetHandle() const { return this->Handle; }

  svtkm::Id GetNumberOfValues() const override { return this->Handle.GetNumberOfValues(); }

  void ReleaseResourcesExecution() override;
  void ReleaseResources() override;

  void Allocate(svtkm::Id numberOfValues) override;
  void Shrink(svtkm::Id numberOfValues) override;

private:
  std::unique_ptr<svtkm::cont::internal::detail::StorageVirtual> MakeNewInstance() const override
  {
    return std::unique_ptr<svtkm::cont::internal::detail::StorageVirtual>(
      new StorageVirtualImpl<T, S>{ svtkm::cont::ArrayHandle<T, S>{} });
  }


  void ControlPortalForInput(svtkm::cont::internal::TransferInfoArray& payload) const override;
  void ControlPortalForOutput(svtkm::cont::internal::TransferInfoArray& payload) override;

  void TransferPortalForInput(svtkm::cont::internal::TransferInfoArray& payload,
                              svtkm::cont::DeviceAdapterId devId) const override;

  void TransferPortalForOutput(svtkm::cont::internal::TransferInfoArray& payload,
                               OutputMode mode,
                               svtkm::Id numberOfValues,
                               svtkm::cont::DeviceAdapterId devId) override;

  svtkm::cont::ArrayHandle<T, S> Handle;
};

} // namespace detail

template <typename T>
class SVTKM_ALWAYS_EXPORT Storage<T, svtkm::cont::StorageTagVirtual>
{
public:
  using ValueType = T;

  using PortalType = svtkm::ArrayPortalRef<T>;
  using PortalConstType = svtkm::ArrayPortalRef<T>;

  Storage() = default;

  // Should never really be used, but just in case.
  Storage(const Storage<T, svtkm::cont::StorageTagVirtual>&) = default;

  template <typename S>
  Storage(const svtkm::cont::ArrayHandle<T, S>& srcArray)
    : VirtualStorage(std::make_shared<detail::StorageVirtualImpl<T, S>>(srcArray))
  {
  }

  ~Storage() = default;

  PortalType GetPortal()
  {
    return make_ArrayPortalRef(
      static_cast<const svtkm::ArrayPortalVirtual<T>*>(this->VirtualStorage->GetPortalControl()),
      this->GetNumberOfValues());
  }

  PortalConstType GetPortalConst() const
  {
    return make_ArrayPortalRef(static_cast<const svtkm::ArrayPortalVirtual<T>*>(
                                 this->VirtualStorage->GetPortalConstControl()),
                               this->GetNumberOfValues());
  }

  svtkm::Id GetNumberOfValues() const { return this->VirtualStorage->GetNumberOfValues(); }

  void Allocate(svtkm::Id numberOfValues);

  void Shrink(svtkm::Id numberOfValues);

  void ReleaseResources();

  Storage<T, svtkm::cont::StorageTagVirtual> NewInstance() const;

  const detail::StorageVirtual* GetStorageVirtual() const { return this->VirtualStorage.get(); }
  detail::StorageVirtual* GetStorageVirtual() { return this->VirtualStorage.get(); }

private:
  std::shared_ptr<detail::StorageVirtual> VirtualStorage;

  Storage(std::shared_ptr<detail::StorageVirtual> virtualStorage)
    : VirtualStorage(virtualStorage)
  {
  }
};

} // namespace internal
}
} // namespace svtkm::cont

#ifndef svtk_m_cont_StorageVirtual_hxx
#include <svtkm/cont/StorageVirtual.hxx>
#endif

#endif
