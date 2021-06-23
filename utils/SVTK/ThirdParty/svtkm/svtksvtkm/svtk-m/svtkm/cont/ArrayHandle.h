//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandle_h
#define svtk_m_cont_ArrayHandle_h

#include <svtkm/cont/svtkm_cont_export.h>

#include <svtkm/Assert.h>
#include <svtkm/Flags.h>
#include <svtkm/Types.h>

#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/ErrorInternal.h>
#include <svtkm/cont/SerializableTypeString.h>
#include <svtkm/cont/Serialization.h>
#include <svtkm/cont/Storage.h>
#include <svtkm/cont/StorageBasic.h>

#include <svtkm/internal/ArrayPortalHelpers.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <mutex>
#include <vector>

#include <svtkm/cont/internal/ArrayHandleExecutionManager.h>
#include <svtkm/cont/internal/ArrayPortalFromIterators.h>

namespace svtkm
{
namespace cont
{

namespace internal
{

/// \brief Base class of all ArrayHandle classes.
///
/// This is an empty class that is used to check if something is an \c
/// ArrayHandle class (or at least something that behaves exactly like one).
/// The \c ArrayHandle template class inherits from this.
///
class SVTKM_CONT_EXPORT ArrayHandleBase
{
};

/// Checks to see if the given type and storage forms a valid array handle
/// (some storage objects cannot support all types). This check is compatible
/// with C++11 type_traits.
///
template <typename T, typename StorageTag>
using IsValidArrayHandle =
  std::integral_constant<bool,
                         !(std::is_base_of<svtkm::cont::internal::UndefinedStorage,
                                           svtkm::cont::internal::Storage<T, StorageTag>>::value)>;

/// Checks to see if the given type and storage forms a invalid array handle
/// (some storage objects cannot support all types). This check is compatible
/// with C++11 type_traits.
///
template <typename T, typename StorageTag>
using IsInValidArrayHandle =
  std::integral_constant<bool, !IsValidArrayHandle<T, StorageTag>::value>;

/// Checks to see if the ArrayHandle allows writing, as some ArrayHandles
/// (Implicit) don't support writing. These will be defined as either
/// std::true_type or std::false_type.
///
/// \sa svtkm::internal::PortalSupportsSets
///
template <typename ArrayHandle>
using IsWritableArrayHandle =
  svtkm::internal::PortalSupportsSets<typename std::decay<ArrayHandle>::type::PortalControl>;
/// @}

/// Checks to see if the given object is an array handle. This check is
/// compatible with C++11 type_traits. It a typedef named \c type that is
/// either std::true_type or std::false_type. Both of these have a typedef
/// named value with the respective boolean value.
///
/// Unlike \c IsValidArrayHandle, if an \c ArrayHandle is used with this
/// class, then it must be created by the compiler and therefore must already
/// be valid. Where \c IsValidArrayHandle is used when you know something is
/// an \c ArrayHandle but you are not sure if the \c StorageTag is valid, this
/// class is used to ensure that a given type is an \c ArrayHandle. It is
/// used internally in the SVTKM_IS_ARRAY_HANDLE macro.
///
template <typename T>
struct ArrayHandleCheck
{
  using U = typename std::remove_pointer<T>::type;
  using type = typename std::is_base_of<::svtkm::cont::internal::ArrayHandleBase, U>::type;
};

#define SVTKM_IS_ARRAY_HANDLE(T)                                                                    \
  SVTKM_STATIC_ASSERT(::svtkm::cont::internal::ArrayHandleCheck<T>::type::value)

} // namespace internal

namespace detail
{

template <typename T>
struct GetTypeInParentheses;
template <typename T>
struct GetTypeInParentheses<void(T)>
{
  using type = T;
};

} // namespace detail

// Implementation for SVTKM_ARRAY_HANDLE_SUBCLASS macros
#define SVTK_M_ARRAY_HANDLE_SUBCLASS_IMPL(classname, fullclasstype, superclass, typename__)         \
  using Thisclass = typename__ svtkm::cont::detail::GetTypeInParentheses<void fullclasstype>::type; \
  using Superclass = typename__ svtkm::cont::detail::GetTypeInParentheses<void superclass>::type;   \
                                                                                                   \
  SVTKM_IS_ARRAY_HANDLE(Superclass);                                                                \
                                                                                                   \
  SVTKM_CONT                                                                                        \
  classname()                                                                                      \
    : Superclass()                                                                                 \
  {                                                                                                \
  }                                                                                                \
                                                                                                   \
  SVTKM_CONT                                                                                        \
  classname(const Thisclass& src)                                                                  \
    : Superclass(src)                                                                              \
  {                                                                                                \
  }                                                                                                \
                                                                                                   \
  SVTKM_CONT                                                                                        \
  classname(Thisclass&& src) noexcept : Superclass(std::move(src)) {}                              \
                                                                                                   \
  SVTKM_CONT                                                                                        \
  classname(const svtkm::cont::ArrayHandle<typename__ Superclass::ValueType,                        \
                                          typename__ Superclass::StorageTag>& src)                 \
    : Superclass(src)                                                                              \
  {                                                                                                \
  }                                                                                                \
                                                                                                   \
  SVTKM_CONT                                                                                        \
  classname(svtkm::cont::ArrayHandle<typename__ Superclass::ValueType,                              \
                                    typename__ Superclass::StorageTag>&& src) noexcept             \
    : Superclass(std::move(src))                                                                   \
  {                                                                                                \
  }                                                                                                \
                                                                                                   \
  SVTKM_CONT                                                                                        \
  Thisclass& operator=(const Thisclass& src)                                                       \
  {                                                                                                \
    this->Superclass::operator=(src);                                                              \
    return *this;                                                                                  \
  }                                                                                                \
                                                                                                   \
  SVTKM_CONT                                                                                        \
  Thisclass& operator=(Thisclass&& src) noexcept                                                   \
  {                                                                                                \
    this->Superclass::operator=(std::move(src));                                                   \
    return *this;                                                                                  \
  }                                                                                                \
                                                                                                   \
  using ValueType = typename__ Superclass::ValueType;                                              \
  using StorageTag = typename__ Superclass::StorageTag

/// \brief Macro to make default methods in ArrayHandle subclasses.
///
/// This macro defines the default constructors, destructors and assignment
/// operators for ArrayHandle subclasses that are templates. The ArrayHandle
/// subclasses are assumed to be empty convenience classes. The macro should be
/// defined after a \c public: declaration.
///
/// This macro takes three arguments. The first argument is the classname.
/// The second argument is the full class type. The third argument is the
/// superclass type (either \c ArrayHandle or another sublcass). Because
/// C macros do not handle template parameters very well (the preprocessor
/// thinks the template commas are macro argument commas), the second and
/// third arguments must be wrapped in parentheses.
///
/// This macro also defines a Superclass typedef as well as ValueType and
/// StorageTag.
///
/// Note that this macro only works on ArrayHandle subclasses that are
/// templated. For ArrayHandle sublcasses that are not templates, use
/// SVTKM_ARRAY_HANDLE_SUBCLASS_NT.
///
#define SVTKM_ARRAY_HANDLE_SUBCLASS(classname, fullclasstype, superclass)                           \
  SVTK_M_ARRAY_HANDLE_SUBCLASS_IMPL(classname, fullclasstype, superclass, typename)

/// \brief Macro to make default methods in ArrayHandle subclasses.
///
/// This macro defines the default constructors, destructors and assignment
/// operators for ArrayHandle subclasses that are not templates. The
/// ArrayHandle subclasses are assumed to be empty convenience classes. The
/// macro should be defined after a \c public: declaration.
///
/// This macro takes two arguments. The first argument is the classname. The
/// second argument is the superclass type (either \c ArrayHandle or another
/// sublcass). Because C macros do not handle template parameters very well
/// (the preprocessor thinks the template commas are macro argument commas),
/// the second argument must be wrapped in parentheses.
///
/// This macro also defines a Superclass typedef as well as ValueType and
/// StorageTag.
///
/// Note that this macro only works on ArrayHandle subclasses that are not
/// templated. For ArrayHandle sublcasses that are templates, use
/// SVTKM_ARRAY_HANDLE_SUBCLASS.
///
#define SVTKM_ARRAY_HANDLE_SUBCLASS_NT(classname, superclass)                                       \
  SVTK_M_ARRAY_HANDLE_SUBCLASS_IMPL(classname, (classname), superclass, )

/// \brief Manages an array-worth of data.
///
/// \c ArrayHandle manages as array of data that can be manipulated by SVTKm
/// algorithms. The \c ArrayHandle may have up to two copies of the array, one
/// for the control environment and one for the execution environment, although
/// depending on the device and how the array is being used, the \c ArrayHandle
/// will only have one copy when possible.
///
/// An ArrayHandle can be constructed one of two ways. Its default construction
/// creates an empty, unallocated array that can later be allocated and filled
/// either by the user or a SVTKm algorithm. The \c ArrayHandle can also be
/// constructed with iterators to a user's array. In this case the \c
/// ArrayHandle will keep a reference to this array but will throw an exception
/// if asked to re-allocate to a larger size.
///
/// \c ArrayHandle behaves like a shared smart pointer in that when it is copied
/// each copy holds a reference to the same array.  These copies are reference
/// counted so that when all copies of the \c ArrayHandle are destroyed, any
/// allocated memory is released.
///
///
template <typename T, typename StorageTag_ = SVTKM_DEFAULT_STORAGE_TAG>
class SVTKM_ALWAYS_EXPORT ArrayHandle : public internal::ArrayHandleBase
{
private:
  // Basic storage is specialized; this template should not be instantiated
  // for it. Specialization is in ArrayHandleBasicImpl.h
  static_assert(!std::is_same<StorageTag_, StorageTagBasic>::value,
                "StorageTagBasic should not use this implementation.");

  using ExecutionManagerType =
    svtkm::cont::internal::ArrayHandleExecutionManagerBase<T, StorageTag_>;

  using MutexType = std::mutex;
  using LockType = std::unique_lock<MutexType>;

public:
  using StorageType = svtkm::cont::internal::Storage<T, StorageTag_>;
  using ValueType = T;
  using StorageTag = StorageTag_;
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;
  template <typename DeviceAdapterTag>
  struct ExecutionTypes
  {
    using Portal = typename ExecutionManagerType::template ExecutionTypes<DeviceAdapterTag>::Portal;
    using PortalConst =
      typename ExecutionManagerType::template ExecutionTypes<DeviceAdapterTag>::PortalConst;
  };

  /// Constructs an empty ArrayHandle. Typically used for output or
  /// intermediate arrays that will be filled by a SVTKm algorithm.
  ///
  SVTKM_CONT ArrayHandle();

  /// Copy constructor.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated copy constructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ArrayHandle(const svtkm::cont::ArrayHandle<ValueType, StorageTag>& src);

  /// Move constructor.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated move constructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ArrayHandle(svtkm::cont::ArrayHandle<ValueType, StorageTag>&& src) noexcept;

  /// Special constructor for subclass specializations that need to set the
  /// initial state of the control array. When this constructor is used, it
  /// is assumed that the control array is valid.
  ///
  ArrayHandle(const StorageType& storage);


  /// Special constructor for subclass specializations that need to set the
  /// initial state of the control array. When this constructor is used, it
  /// is assumed that the control array is valid.
  ///
  ArrayHandle(StorageType&& storage) noexcept;

  /// Destructs an empty ArrayHandle.
  ///
  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandle();

  /// \brief Copies an ArrayHandle
  ///
  SVTKM_CONT
  svtkm::cont::ArrayHandle<ValueType, StorageTag>& operator=(
    const svtkm::cont::ArrayHandle<ValueType, StorageTag>& src);

  /// \brief Move and Assignment of an ArrayHandle
  ///
  SVTKM_CONT
  svtkm::cont::ArrayHandle<ValueType, StorageTag>& operator=(
    svtkm::cont::ArrayHandle<ValueType, StorageTag>&& src) noexcept;

  /// Like a pointer, two \c ArrayHandles are considered equal if they point
  /// to the same location in memory.
  ///
  SVTKM_CONT
  bool operator==(const ArrayHandle<ValueType, StorageTag>& rhs) const
  {
    return (this->Internals == rhs.Internals);
  }

  SVTKM_CONT
  bool operator!=(const ArrayHandle<ValueType, StorageTag>& rhs) const
  {
    return (this->Internals != rhs.Internals);
  }

  template <typename VT, typename ST>
  SVTKM_CONT bool operator==(const ArrayHandle<VT, ST>&) const
  {
    return false; // different valuetype and/or storage
  }

  template <typename VT, typename ST>
  SVTKM_CONT bool operator!=(const ArrayHandle<VT, ST>&) const
  {
    return true; // different valuetype and/or storage
  }

  /// Get the storage.
  ///
  SVTKM_CONT StorageType& GetStorage();

  /// Get the storage.
  ///
  SVTKM_CONT const StorageType& GetStorage() const;

  /// Get the array portal of the control array.
  /// Since worklet invocations are asynchronous and this routine is a synchronization point,
  /// exceptions maybe thrown for errors from previously executed worklets.
  ///
  SVTKM_CONT PortalControl GetPortalControl();

  /// Get the array portal of the control array.
  /// Since worklet invocations are asynchronous and this routine is a synchronization point,
  /// exceptions maybe thrown for errors from previously executed worklets.
  ///
  SVTKM_CONT PortalConstControl GetPortalConstControl() const;

  /// Returns the number of entries in the array.
  ///
  SVTKM_CONT svtkm::Id GetNumberOfValues() const
  {
    LockType lock = this->GetLock();

    return this->GetNumberOfValues(lock);
  }

  /// \brief Allocates an array large enough to hold the given number of values.
  ///
  /// The allocation may be done on an already existing array, but can wipe out
  /// any data already in the array. This method can throw
  /// ErrorBadAllocation if the array cannot be allocated or
  /// ErrorBadValue if the allocation is not feasible (for example, the
  /// array storage is read-only).
  ///
  SVTKM_CONT
  void Allocate(svtkm::Id numberOfValues)
  {
    LockType lock = this->GetLock();
    this->ReleaseResourcesExecutionInternal(lock);
    this->Internals->GetControlArray(lock)->Allocate(numberOfValues);
    this->Internals->SetControlArrayValid(lock, true);
  }

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  void Shrink(svtkm::Id numberOfValues);

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  SVTKM_CONT void ReleaseResourcesExecution()
  {
    LockType lock = this->GetLock();

    // Save any data in the execution environment by making sure it is synced
    // with the control environment.
    this->SyncControlArray(lock);

    this->ReleaseResourcesExecutionInternal(lock);
  }

  /// Releases all resources in both the control and execution environments.
  ///
  SVTKM_CONT void ReleaseResources()
  {
    LockType lock = this->GetLock();

    this->ReleaseResourcesExecutionInternal(lock);

    if (this->Internals->IsControlArrayValid(lock))
    {
      this->Internals->GetControlArray(lock)->ReleaseResources();
      this->Internals->SetControlArrayValid(lock, false);
    }
  }

  // clang-format off
  /// Prepares this array to be used as an input to an operation in the
  /// execution environment. If necessary, copies data to the execution
  /// environment. Can throw an exception if this array does not yet contain
  /// any data. Returns a portal that can be used in code running in the
  /// execution environment.
  ///
  template <typename DeviceAdapterTag>
  SVTKM_CONT
  typename ExecutionTypes<DeviceAdapterTag>::PortalConst PrepareForInput(DeviceAdapterTag) const;
  // clang-format on

  /// Prepares (allocates) this array to be used as an output from an operation
  /// in the execution environment. The internal state of this class is set to
  /// have valid data in the execution array with the assumption that the array
  /// will be filled soon (i.e. before any other methods of this object are
  /// called). Returns a portal that can be used in code running in the
  /// execution environment.
  ///
  template <typename DeviceAdapterTag>
  SVTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::Portal PrepareForOutput(
    svtkm::Id numberOfValues,
    DeviceAdapterTag);

  /// Prepares this array to be used in an in-place operation (both as input
  /// and output) in the execution environment. If necessary, copies data to
  /// the execution environment. Can throw an exception if this array does not
  /// yet contain any data. Returns a portal that can be used in code running
  /// in the execution environment.
  ///
  template <typename DeviceAdapterTag>
  SVTKM_CONT typename ExecutionTypes<DeviceAdapterTag>::Portal PrepareForInPlace(DeviceAdapterTag);

  /// Returns the DeviceAdapterId for the current device. If there is no device
  /// with an up-to-date copy of the data, SVTKM_DEVICE_ADAPTER_UNDEFINED is
  /// returned.
  ///
  /// Note that in a multithreaded environment the validity of this result can
  /// change.
  SVTKM_CONT
  DeviceAdapterId GetDeviceAdapterId() const
  {
    LockType lock = this->GetLock();
    return this->Internals->IsExecutionArrayValid(lock)
      ? this->Internals->GetExecutionArray(lock)->GetDeviceAdapterId()
      : DeviceAdapterTagUndefined{};
  }

  /// Synchronizes the control array with the execution array. If either the
  /// user array or control array is already valid, this method does nothing
  /// (because the data is already available in the control environment).
  /// Although the internal state of this class can change, the method is
  /// declared const because logically the data does not.
  ///
  SVTKM_CONT void SyncControlArray() const
  {
    LockType lock = this->GetLock();
    this->SyncControlArray(lock);
  }

  // Probably should make this private, but ArrayHandleStreaming needs access.
protected:
  /// Acquires a lock on the internals of this `ArrayHandle`. The calling
  /// function should keep the returned lock and let it go out of scope
  /// when the lock is no longer needed.
  ///
  LockType GetLock() const { return LockType(this->Internals->Mutex); }

  /// Gets this array handle ready to interact with the given device. If the
  /// array handle has already interacted with this device, then this method
  /// does nothing. Although the internal state of this class can change, the
  /// method is declared const because logically the data does not.
  ///
  template <typename DeviceAdapterTag>
  SVTKM_CONT void PrepareForDevice(LockType& lock, DeviceAdapterTag) const;

  /// Synchronizes the control array with the execution array. If either the
  /// user array or control array is already valid, this method does nothing
  /// (because the data is already available in the control environment).
  /// Although the internal state of this class can change, the method is
  /// declared const because logically the data does not.
  ///
  SVTKM_CONT void SyncControlArray(LockType& lock) const;

  svtkm::Id GetNumberOfValues(LockType& lock) const;

  SVTKM_CONT
  void ReleaseResourcesExecutionInternal(LockType& lock)
  {
    if (this->Internals->IsExecutionArrayValid(lock))
    {
      this->Internals->GetExecutionArray(lock)->ReleaseResources();
      this->Internals->SetExecutionArrayValid(lock, false);
    }
  }

  class SVTKM_ALWAYS_EXPORT InternalStruct
  {
    mutable StorageType ControlArray;
    mutable bool ControlArrayValid = false;

    mutable std::unique_ptr<ExecutionManagerType> ExecutionArray;
    mutable bool ExecutionArrayValid = false;

    SVTKM_CONT void CheckLock(const LockType& lock) const
    {
      SVTKM_ASSERT((lock.mutex() == &this->Mutex) && (lock.owns_lock()));
    }

  public:
    MutexType Mutex;

    InternalStruct() = default;
    ~InternalStruct() = default;
    InternalStruct(const StorageType& storage);
    InternalStruct(StorageType&& storage);

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
    SVTKM_CONT StorageType* GetControlArray(const LockType& lock) const
    {
      this->CheckLock(lock);
      return &this->ControlArray;
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
    SVTKM_CONT ExecutionManagerType* GetExecutionArray(const LockType& lock) const
    {
      this->CheckLock(lock);
      return this->ExecutionArray.get();
    }
    SVTKM_CONT void DeleteExecutionArray(const LockType& lock)
    {
      this->CheckLock(lock);
      this->ExecutionArray.reset();
      this->ExecutionArrayValid = false;
    }
    template <typename DeviceAdapterTag>
    SVTKM_CONT void NewExecutionArray(const LockType& lock, DeviceAdapterTag)
    {
      SVTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);
      this->CheckLock(lock);
      SVTKM_ASSERT(this->ExecutionArray == nullptr);
      SVTKM_ASSERT(!this->ExecutionArrayValid);
      this->ExecutionArray.reset(
        new svtkm::cont::internal::ArrayHandleExecutionManager<T, StorageTag, DeviceAdapterTag>(
          &this->ControlArray));
    }
  };

  SVTKM_CONT
  ArrayHandle(const std::shared_ptr<InternalStruct>& i)
    : Internals(i)
  {
  }

  std::shared_ptr<InternalStruct> Internals;
};

/// A convenience function for creating an ArrayHandle from a standard C array.
///
template <typename T>
SVTKM_CONT svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic>
make_ArrayHandle(const T* array, svtkm::Id length, svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
{
  using ArrayHandleType = svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic>;
  if (copy == svtkm::CopyFlag::On)
  {
    ArrayHandleType handle;
    handle.Allocate(length);
    std::copy(
      array, array + length, svtkm::cont::ArrayPortalToIteratorBegin(handle.GetPortalControl()));
    return handle;
  }
  else
  {
    using StorageType = svtkm::cont::internal::Storage<T, svtkm::cont::StorageTagBasic>;
    return ArrayHandleType(StorageType(array, length));
  }
}

/// A convenience function for creating an ArrayHandle from an std::vector.
///
template <typename T, typename Allocator>
SVTKM_CONT svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic> make_ArrayHandle(
  const std::vector<T, Allocator>& array,
  svtkm::CopyFlag copy = svtkm::CopyFlag::Off)
{
  if (!array.empty())
  {
    return make_ArrayHandle(&array.front(), static_cast<svtkm::Id>(array.size()), copy);
  }
  else
  {
    // Vector empty. Just return an empty array handle.
    return svtkm::cont::ArrayHandle<T, svtkm::cont::StorageTagBasic>();
  }
}

namespace detail
{

template <typename T>
SVTKM_NEVER_EXPORT SVTKM_CONT inline void
printSummary_ArrayHandle_Value(const T& value, std::ostream& out, svtkm::VecTraitsTagSingleComponent)
{
  out << value;
}

SVTKM_NEVER_EXPORT
SVTKM_CONT
inline void printSummary_ArrayHandle_Value(svtkm::UInt8 value,
                                           std::ostream& out,
                                           svtkm::VecTraitsTagSingleComponent)
{
  out << static_cast<int>(value);
}

SVTKM_NEVER_EXPORT
SVTKM_CONT
inline void printSummary_ArrayHandle_Value(svtkm::Int8 value,
                                           std::ostream& out,
                                           svtkm::VecTraitsTagSingleComponent)
{
  out << static_cast<int>(value);
}

template <typename T>
SVTKM_NEVER_EXPORT SVTKM_CONT inline void printSummary_ArrayHandle_Value(
  const T& value,
  std::ostream& out,
  svtkm::VecTraitsTagMultipleComponents)
{
  using Traits = svtkm::VecTraits<T>;
  using ComponentType = typename Traits::ComponentType;
  using IsVecOfVec = typename svtkm::VecTraits<ComponentType>::HasMultipleComponents;
  svtkm::IdComponent numComponents = Traits::GetNumberOfComponents(value);
  out << "(";
  printSummary_ArrayHandle_Value(Traits::GetComponent(value, 0), out, IsVecOfVec());
  for (svtkm::IdComponent index = 1; index < numComponents; ++index)
  {
    out << ",";
    printSummary_ArrayHandle_Value(Traits::GetComponent(value, index), out, IsVecOfVec());
  }
  out << ")";
}

template <typename T1, typename T2>
SVTKM_NEVER_EXPORT SVTKM_CONT inline void printSummary_ArrayHandle_Value(
  const svtkm::Pair<T1, T2>& value,
  std::ostream& out,
  svtkm::VecTraitsTagSingleComponent)
{
  out << "{";
  printSummary_ArrayHandle_Value(
    value.first, out, typename svtkm::VecTraits<T1>::HasMultipleComponents());
  out << ",";
  printSummary_ArrayHandle_Value(
    value.second, out, typename svtkm::VecTraits<T2>::HasMultipleComponents());
  out << "}";
}



} // namespace detail

template <typename T, typename StorageT>
SVTKM_NEVER_EXPORT SVTKM_CONT inline void printSummary_ArrayHandle(
  const svtkm::cont::ArrayHandle<T, StorageT>& array,
  std::ostream& out,
  bool full = false)
{
  using ArrayType = svtkm::cont::ArrayHandle<T, StorageT>;
  using PortalType = typename ArrayType::PortalConstControl;
  using IsVec = typename svtkm::VecTraits<T>::HasMultipleComponents;

  svtkm::Id sz = array.GetNumberOfValues();

  out << "valueType=" << typeid(T).name() << " storageType=" << typeid(StorageT).name()
      << " numValues=" << sz << " bytes=" << (static_cast<size_t>(sz) * sizeof(T)) << " [";

  PortalType portal = array.GetPortalConstControl();
  if (full || sz <= 7)
  {
    for (svtkm::Id i = 0; i < sz; i++)
    {
      detail::printSummary_ArrayHandle_Value(portal.Get(i), out, IsVec());
      if (i != (sz - 1))
      {
        out << " ";
      }
    }
  }
  else
  {
    detail::printSummary_ArrayHandle_Value(portal.Get(0), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(1), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(2), out, IsVec());
    out << " ... ";
    detail::printSummary_ArrayHandle_Value(portal.Get(sz - 3), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(sz - 2), out, IsVec());
    out << " ";
    detail::printSummary_ArrayHandle_Value(portal.Get(sz - 1), out, IsVec());
  }
  out << "]\n";
}
}
} //namespace svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <typename T>
struct SerializableTypeString<ArrayHandle<T>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};

namespace internal
{

template <typename T, typename S>
void SVTKM_CONT ArrayHandleDefaultSerialization(svtkmdiy::BinaryBuffer& bb,
                                               const svtkm::cont::ArrayHandle<T, S>& obj);

} // internal
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename T>
struct Serialization<svtkm::cont::ArrayHandle<T>>
{
  static SVTKM_CONT void save(BinaryBuffer& bb, const svtkm::cont::ArrayHandle<T>& obj)
  {
    svtkm::cont::internal::ArrayHandleDefaultSerialization(bb, obj);
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, svtkm::cont::ArrayHandle<T>& obj);
};

} // diy
/// @endcond SERIALIZATION

#ifndef svtk_m_cont_ArrayHandle_hxx
#include <svtkm/cont/ArrayHandle.hxx>
#endif

#ifndef svtk_m_cont_internal_ArrayHandleBasicImpl_h
#include <svtkm/cont/internal/ArrayHandleBasicImpl.h>
#endif

#include <svtkm/cont/internal/ArrayExportMacros.h>

#ifndef svtkm_cont_ArrayHandle_cxx

namespace svtkm
{
namespace cont
{

#define SVTKM_ARRAYHANDLE_EXPORT(Type)                                                              \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<Type, StorageTagBasic>;              \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayHandle<svtkm::Vec<Type, 2>, StorageTagBasic>;                                              \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT                                                  \
    ArrayHandle<svtkm::Vec<Type, 3>, StorageTagBasic>;                                              \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT ArrayHandle<svtkm::Vec<Type, 4>, StorageTagBasic>;

SVTKM_ARRAYHANDLE_EXPORT(char)
SVTKM_ARRAYHANDLE_EXPORT(svtkm::Int8)
SVTKM_ARRAYHANDLE_EXPORT(svtkm::UInt8)
SVTKM_ARRAYHANDLE_EXPORT(svtkm::Int16)
SVTKM_ARRAYHANDLE_EXPORT(svtkm::UInt16)
SVTKM_ARRAYHANDLE_EXPORT(svtkm::Int32)
SVTKM_ARRAYHANDLE_EXPORT(svtkm::UInt32)
SVTKM_ARRAYHANDLE_EXPORT(svtkm::Int64)
SVTKM_ARRAYHANDLE_EXPORT(svtkm::UInt64)
SVTKM_ARRAYHANDLE_EXPORT(svtkm::Float32)
SVTKM_ARRAYHANDLE_EXPORT(svtkm::Float64)

#undef SVTKM_ARRAYHANDLE_EXPORT
}
} // end svtkm::cont

#endif // !svtkm_cont_ArrayHandle_cxx

#endif //svtk_m_cont_ArrayHandle_h
