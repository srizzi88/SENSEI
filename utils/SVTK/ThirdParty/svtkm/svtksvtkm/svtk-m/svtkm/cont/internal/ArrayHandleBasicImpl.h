//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_internal_ArrayHandleBasicImpl_h
#define svtk_m_cont_internal_ArrayHandleBasicImpl_h

#include <svtkm/cont/ArrayHandle.h>

#include <svtkm/cont/StorageBasic.h>

#include <type_traits>

namespace svtkm
{
namespace cont
{

namespace internal
{

struct ArrayHandleImpl;

/// Type-agnostic container for an execution memory buffer.
struct SVTKM_CONT_EXPORT TypelessExecutionArray
{

  TypelessExecutionArray(void*& executionArray,
                         void*& executionArrayEnd,
                         void*& executionArrayCapacity,
                         const StorageBasicBase* controlArray);

  void*& Array;
  void*& ArrayEnd;
  void*& ArrayCapacity;
  // Used by cuda to detect and share managed memory allocations.
  const void* ArrayControl;
  const void* ArrayControlCapacity;
};

/// Factory that generates execution portals for basic storage.
template <typename T, typename DeviceTag>
struct ExecutionPortalFactoryBasic
#ifndef SVTKM_DOXYGEN_ONLY
  ;
#else  // SVTKM_DOXYGEN_ONLY
{
  /// The portal type.
  using PortalType = SomePortalType;

  /// The cont portal type.
  using ConstPortalType = SomePortalType;

  /// Create a portal to access the execution data from @a start to @a end.
  SVTKM_CONT
  static PortalType CreatePortal(ValueType* start, ValueType* end);

  /// Create a const portal to access the execution data from @a start to @a end.
  SVTKM_CONT
  static PortalConstType CreatePortalConst(const ValueType* start, const ValueType* end);
};
#endif // SVTKM_DOXYGEN_ONLY

/// Typeless interface for interacting with a execution memory buffer when using basic storage.
struct SVTKM_CONT_EXPORT ExecutionArrayInterfaceBasicBase
{
  SVTKM_CONT explicit ExecutionArrayInterfaceBasicBase(StorageBasicBase& storage);
  SVTKM_CONT virtual ~ExecutionArrayInterfaceBasicBase();

  SVTKM_CONT
  virtual DeviceAdapterId GetDeviceId() const = 0;

  /// If @a execArray's base pointer is null, allocate a new buffer.
  /// If (capacity - base) < @a numBytes, the buffer will be freed and
  /// reallocated. If (capacity - base) >= numBytes, a new end is marked.
  SVTKM_CONT
  virtual void Allocate(TypelessExecutionArray& execArray,
                        svtkm::Id numberOfValues,
                        svtkm::UInt64 sizeOfValue) const = 0;

  /// Release the buffer held by @a execArray and reset all pointer to null.
  SVTKM_CONT
  virtual void Free(TypelessExecutionArray& execArray) const = 0;

  /// Copy @a numBytes from @a controlPtr to @a executionPtr.
  SVTKM_CONT
  virtual void CopyFromControl(const void* controlPtr,
                               void* executionPtr,
                               svtkm::UInt64 numBytes) const = 0;

  /// Copy @a numBytes from @a executionPtr to @a controlPtr.
  SVTKM_CONT
  virtual void CopyToControl(const void* executionPtr,
                             void* controlPtr,
                             svtkm::UInt64 numBytes) const = 0;


  SVTKM_CONT virtual void UsingForRead(const void* controlPtr,
                                      const void* executionPtr,
                                      svtkm::UInt64 numBytes) const = 0;
  SVTKM_CONT virtual void UsingForWrite(const void* controlPtr,
                                       const void* executionPtr,
                                       svtkm::UInt64 numBytes) const = 0;
  SVTKM_CONT virtual void UsingForReadWrite(const void* controlPtr,
                                           const void* executionPtr,
                                           svtkm::UInt64 numBytes) const = 0;

protected:
  StorageBasicBase& ControlStorage;
};

/**
 * Specializations should inherit from and implement the API of
 * ExecutionArrayInterfaceBasicBase.
 */
template <typename DeviceTag>
struct ExecutionArrayInterfaceBasic;

struct SVTKM_CONT_EXPORT ArrayHandleImpl
{
  using MutexType = std::mutex;
  using LockType = std::unique_lock<MutexType>;

  SVTKM_CONT
  template <typename T>
  explicit ArrayHandleImpl(T t)
    : Internals(new InternalStruct(t))
  {
  }

  SVTKM_CONT
  template <typename T>
  explicit ArrayHandleImpl(
    const svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagBasic>& storage)
    : Internals(new InternalStruct(storage))
  {
  }

  SVTKM_CONT
  template <typename T>
  explicit ArrayHandleImpl(svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagBasic>&& storage)
    : Internals(new InternalStruct(std::move(storage)))
  {
  }

  SVTKM_CONT ~ArrayHandleImpl() = default;

  SVTKM_CONT ArrayHandleImpl(const ArrayHandleImpl&) = delete;
  SVTKM_CONT void operator=(const ArrayHandleImpl&) = delete;

  //Throws ErrorInternal if ControlArrayValid == false
  SVTKM_CONT void CheckControlArrayValid(const LockType& lock) noexcept(false);

  SVTKM_CONT svtkm::Id GetNumberOfValues(const LockType& lock, svtkm::UInt64 sizeOfT) const;
  SVTKM_CONT void Allocate(const LockType& lock, svtkm::Id numberOfValues, svtkm::UInt64 sizeOfT);
  SVTKM_CONT void Shrink(const LockType& lock, svtkm::Id numberOfValues, svtkm::UInt64 sizeOfT);

  SVTKM_CONT void SyncControlArray(const LockType& lock, svtkm::UInt64 sizeofT) const;
  SVTKM_CONT void ReleaseResources(const LockType& lock);
  SVTKM_CONT void ReleaseResourcesExecutionInternal(const LockType& lock);

  SVTKM_CONT void PrepareForInput(const LockType& lock, svtkm::UInt64 sizeofT) const;
  SVTKM_CONT void PrepareForOutput(const LockType& lock, svtkm::Id numVals, svtkm::UInt64 sizeofT);
  SVTKM_CONT void PrepareForInPlace(const LockType& lock, svtkm::UInt64 sizeofT);

  // Check if the current device matches the last one. If they don't match
  // this moves all data back from execution environment and deletes the
  // ExecutionInterface instance.
  // Returns true when the caller needs to reallocate ExecutionInterface
  SVTKM_CONT bool PrepareForDevice(const LockType& lock,
                                  DeviceAdapterId devId,
                                  svtkm::UInt64 sizeofT) const;

  SVTKM_CONT DeviceAdapterId GetDeviceAdapterId(const LockType& lock) const;

  /// Acquires a lock on the internals of this `ArrayHandle`. The calling
  /// function should keep the returned lock and let it go out of scope
  /// when the lock is no longer needed.
  ///
  LockType GetLock() const { return LockType(this->Internals->Mutex); }

  class SVTKM_CONT_EXPORT InternalStruct
  {
    mutable bool ControlArrayValid;
    StorageBasicBase* ControlArray;

    mutable ExecutionArrayInterfaceBasicBase* ExecutionInterface;
    mutable bool ExecutionArrayValid;
    mutable void* ExecutionArray;
    mutable void* ExecutionArrayEnd;
    mutable void* ExecutionArrayCapacity;

    SVTKM_CONT void CheckLock(const LockType& lock) const
    {
      SVTKM_ASSERT((lock.mutex() == &this->Mutex) && (lock.owns_lock()));
    }

  public:
    MutexType Mutex;

    template <typename T>
    SVTKM_CONT explicit InternalStruct(T)
      : ControlArrayValid(false)
      , ControlArray(new svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagBasic>())
      , ExecutionInterface(nullptr)
      , ExecutionArrayValid(false)
      , ExecutionArray(nullptr)
      , ExecutionArrayEnd(nullptr)
      , ExecutionArrayCapacity(nullptr)
    {
    }

    template <typename T>
    SVTKM_CONT explicit InternalStruct(
      const svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagBasic>& storage)
      : ControlArrayValid(true)
      , ControlArray(new svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagBasic>(storage))
      , ExecutionInterface(nullptr)
      , ExecutionArrayValid(false)
      , ExecutionArray(nullptr)
      , ExecutionArrayEnd(nullptr)
      , ExecutionArrayCapacity(nullptr)
    {
    }

    SVTKM_CONT
    template <typename T>
    explicit InternalStruct(svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagBasic>&& storage)
      : ControlArrayValid(true)
      , ControlArray(
          new svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagBasic>(std::move(storage)))
      , ExecutionInterface(nullptr)
      , ExecutionArrayValid(false)
      , ExecutionArray(nullptr)
      , ExecutionArrayEnd(nullptr)
      , ExecutionArrayCapacity(nullptr)
    {
    }

    ~InternalStruct();

    // To access any feature in InternalStruct, you must have locked the mutex. You have
    // to prove it by passing in a reference to a std::unique_lock.
    SVTKM_CONT bool IsControlArrayValid(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ControlArrayValid;
    }
    SVTKM_CONT void SetControlArrayValid(const LockType& lock, bool value)
    {
      this->CheckLock(lock);
      this->ControlArrayValid = value;
    }
    SVTKM_CONT StorageBasicBase* GetControlArray(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ControlArray;
    }
    SVTKM_CONT bool IsExecutionArrayValid(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionArrayValid;
    }
    SVTKM_CONT void SetExecutionArrayValid(const LockType& lock, bool value)
    {
      this->CheckLock(lock);
      this->ExecutionArrayValid = value;
    }
    SVTKM_CONT ExecutionArrayInterfaceBasicBase* GetExecutionInterface(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionInterface;
    }
    SVTKM_CONT void SetExecutionInterface(const LockType& lock,
                                         ExecutionArrayInterfaceBasicBase* executionInterface) const
    {
      this->CheckLock(lock);
      if (this->ExecutionInterface != nullptr)
      {
        delete this->ExecutionInterface;
        this->ExecutionInterface = nullptr;
      }
      this->ExecutionInterface = executionInterface;
    }
    SVTKM_CONT void*& GetExecutionArray(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionArray;
    }
    SVTKM_CONT void*& GetExecutionArrayEnd(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionArrayEnd;
    }
    SVTKM_CONT void*& GetExecutionArrayCapacity(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionArrayCapacity;
    }

    SVTKM_CONT TypelessExecutionArray MakeTypelessExecutionArray(const LockType& lock);
  };

  std::shared_ptr<InternalStruct> Internals;
};

} // end namespace internal

/// Specialization of ArrayHandle for Basic storage. The goal here is to reduce
/// the amount of codegen for the common case of Basic storage when we build
/// the common arrays into libsvtkm_cont.
template <typename T>
class SVTKM_ALWAYS_EXPORT ArrayHandle<T, ::svtkm::cont::StorageTagBasic>
  : public ::svtkm::cont::internal::ArrayHandleBase
{
private:
  using Thisclass = ArrayHandle<T, ::svtkm::cont::StorageTagBasic>;

  template <typename DeviceTag>
  using PortalFactory = svtkm::cont::internal::ExecutionPortalFactoryBasic<T, DeviceTag>;

  using MutexType = internal::ArrayHandleImpl::MutexType;
  using LockType = internal::ArrayHandleImpl::LockType;

public:
  using StorageTag = ::svtkm::cont::StorageTagBasic;
  using StorageType = svtkm::cont::internal::Storage<T, StorageTag>;
  using ValueType = T;
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  template <typename DeviceTag>
  struct ExecutionTypes
  {
    SVTKM_IS_DEVICE_ADAPTER_TAG(DeviceTag);
    using Portal = typename PortalFactory<DeviceTag>::PortalType;
    using PortalConst = typename PortalFactory<DeviceTag>::PortalConstType;
  };

  SVTKM_CONT ArrayHandle();
  SVTKM_CONT ArrayHandle(const Thisclass& src);
  SVTKM_CONT ArrayHandle(Thisclass&& src) noexcept;

  SVTKM_CONT ArrayHandle(const StorageType& storage) noexcept;
  SVTKM_CONT ArrayHandle(StorageType&& storage) noexcept;

  SVTKM_CONT ~ArrayHandle();

  SVTKM_CONT Thisclass& operator=(const Thisclass& src);
  SVTKM_CONT Thisclass& operator=(Thisclass&& src) noexcept;

  SVTKM_CONT bool operator==(const Thisclass& rhs) const;
  SVTKM_CONT bool operator!=(const Thisclass& rhs) const;

  template <typename VT, typename ST>
  SVTKM_CONT bool operator==(const ArrayHandle<VT, ST>&) const;
  template <typename VT, typename ST>
  SVTKM_CONT bool operator!=(const ArrayHandle<VT, ST>&) const;

  SVTKM_CONT StorageType& GetStorage();
  SVTKM_CONT const StorageType& GetStorage() const;
  SVTKM_CONT PortalControl GetPortalControl();
  SVTKM_CONT PortalConstControl GetPortalConstControl() const;
  SVTKM_CONT svtkm::Id GetNumberOfValues() const;

  SVTKM_CONT void Allocate(svtkm::Id numberOfValues);
  SVTKM_CONT void Shrink(svtkm::Id numberOfValues);
  SVTKM_CONT void ReleaseResourcesExecution();
  SVTKM_CONT void ReleaseResources();

  template <typename DeviceAdapterTag>
  SVTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::PortalConst PrepareForInput(
    DeviceAdapterTag device) const;

  template <typename DeviceAdapterTag>
  SVTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::Portal PrepareForOutput(
    svtkm::Id numVals,
    DeviceAdapterTag device);

  template <typename DeviceAdapterTag>
  SVTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::Portal PrepareForInPlace(
    DeviceAdapterTag device);

  template <typename DeviceAdapterTag>
  SVTKM_CONT void PrepareForDevice(const LockType& lock, DeviceAdapterTag) const;

  SVTKM_CONT DeviceAdapterId GetDeviceAdapterId() const;

  SVTKM_CONT void SyncControlArray() const;

  std::shared_ptr<internal::ArrayHandleImpl> Internals;

private:
  SVTKM_CONT void SyncControlArray(const LockType& lock) const;
  SVTKM_CONT void ReleaseResourcesExecutionInternal(const LockType& lock);

  /// Acquires a lock on the internals of this `ArrayHandle`. The calling
  /// function should keep the returned lock and let it go out of scope
  /// when the lock is no longer needed.
  ///
  LockType GetLock() const { return this->Internals->GetLock(); }
};

} // end namespace cont
} // end namespace svtkm

#ifndef svtkm_cont_internal_ArrayHandleImpl_cxx
#ifdef SVTKM_MSVC
extern template class SVTKM_CONT_TEMPLATE_EXPORT
  std::shared_ptr<svtkm::cont::internal::ArrayHandleImpl>;
#endif
#endif

#ifndef svtk_m_cont_internal_ArrayHandleBasicImpl_hxx
#include <svtkm/cont/internal/ArrayHandleBasicImpl.hxx>
#endif

#endif // svtk_m_cont_internal_ArrayHandleBasicImpl_h
