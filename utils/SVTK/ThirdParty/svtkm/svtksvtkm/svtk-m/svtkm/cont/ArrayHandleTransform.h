//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleTransform_h
#define svtk_m_cont_ArrayHandleTransform_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ErrorBadType.h>
#include <svtkm/cont/ErrorInternal.h>
#include <svtkm/cont/ExecutionAndControlObjectBase.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>

#include <svtkm/internal/ArrayPortalHelpers.h>

#include <svtkm/cont/serial/internal/DeviceAdapterRuntimeDetectorSerial.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

/// Tag used in place of an inverse functor.
struct NullFunctorType
{
};
}
}
} // namespace svtkm::cont::internal

namespace svtkm
{
namespace exec
{
namespace internal
{

using NullFunctorType = svtkm::cont::internal::NullFunctorType;

/// \brief An array portal that transforms a value from another portal.
///
template <typename ValueType_,
          typename PortalType_,
          typename FunctorType_,
          typename InverseFunctorType_ = NullFunctorType>
class SVTKM_ALWAYS_EXPORT ArrayPortalTransform;

template <typename ValueType_, typename PortalType_, typename FunctorType_>
class SVTKM_ALWAYS_EXPORT
  ArrayPortalTransform<ValueType_, PortalType_, FunctorType_, NullFunctorType>
{
public:
  using PortalType = PortalType_;
  using ValueType = ValueType_;
  using FunctorType = FunctorType_;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalTransform(const PortalType& portal = PortalType(),
                       const FunctorType& functor = FunctorType())
    : Portal(portal)
    , Functor(functor)
  {
  }

  /// Copy constructor for any other ArrayPortalTransform with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <class OtherV, class OtherP, class OtherF>
  SVTKM_EXEC_CONT ArrayPortalTransform(const ArrayPortalTransform<OtherV, OtherP, OtherF>& src)
    : Portal(src.GetPortal())
    , Functor(src.GetFunctor())
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ValueType Get(svtkm::Id index) const { return this->Functor(this->Portal.Get(index)); }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  const PortalType& GetPortal() const { return this->Portal; }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  const FunctorType& GetFunctor() const { return this->Functor; }

protected:
  PortalType Portal;
  FunctorType Functor;
};

template <typename ValueType_,
          typename PortalType_,
          typename FunctorType_,
          typename InverseFunctorType_>
class SVTKM_ALWAYS_EXPORT ArrayPortalTransform
  : public ArrayPortalTransform<ValueType_, PortalType_, FunctorType_, NullFunctorType>
{
  using Writable = svtkm::internal::PortalSupportsSets<PortalType_>;

public:
  using Superclass = ArrayPortalTransform<ValueType_, PortalType_, FunctorType_, NullFunctorType>;
  using PortalType = PortalType_;
  using ValueType = ValueType_;
  using FunctorType = FunctorType_;
  using InverseFunctorType = InverseFunctorType_;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalTransform(const PortalType& portal = PortalType(),
                       const FunctorType& functor = FunctorType(),
                       const InverseFunctorType& inverseFunctor = InverseFunctorType())
    : Superclass(portal, functor)
    , InverseFunctor(inverseFunctor)
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <class OtherV, class OtherP, class OtherF, class OtherInvF>
  SVTKM_EXEC_CONT ArrayPortalTransform(
    const ArrayPortalTransform<OtherV, OtherP, OtherF, OtherInvF>& src)
    : Superclass(src)
    , InverseFunctor(src.GetInverseFunctor())
  {
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Writable_ = Writable,
            typename = typename std::enable_if<Writable_::value>::type>
  SVTKM_EXEC_CONT void Set(svtkm::Id index, const ValueType& value) const
  {
    this->Portal.Set(index, this->InverseFunctor(value));
  }

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  const InverseFunctorType& GetInverseFunctor() const { return this->InverseFunctor; }

private:
  InverseFunctorType InverseFunctor;
};
}
}
} // namespace svtkm::exec::internal

namespace svtkm
{
namespace cont
{

namespace internal
{

template <typename ProvidedFunctorType, typename FunctorIsExecContObject>
struct TransformFunctorManagerImpl;

template <typename ProvidedFunctorType>
struct TransformFunctorManagerImpl<ProvidedFunctorType, std::false_type>
{
  SVTKM_STATIC_ASSERT_MSG(!svtkm::cont::internal::IsExecutionObjectBase<ProvidedFunctorType>::value,
                         "Must use an ExecutionAndControlObject instead of an ExecutionObject.");

  ProvidedFunctorType Functor;
  using FunctorType = ProvidedFunctorType;

  TransformFunctorManagerImpl() = default;

  SVTKM_CONT
  TransformFunctorManagerImpl(const ProvidedFunctorType& functor)
    : Functor(functor)
  {
  }

  SVTKM_CONT
  ProvidedFunctorType PrepareForControl() const { return this->Functor; }

  template <typename Device>
  SVTKM_CONT ProvidedFunctorType PrepareForExecution(Device) const
  {
    return this->Functor;
  }
};

template <typename ProvidedFunctorType>
struct TransformFunctorManagerImpl<ProvidedFunctorType, std::true_type>
{
  SVTKM_IS_EXECUTION_AND_CONTROL_OBJECT(ProvidedFunctorType);

  ProvidedFunctorType Functor;
  //  using FunctorType = decltype(std::declval<ProvidedFunctorType>().PrepareForControl());
  using FunctorType = decltype(Functor.PrepareForControl());

  TransformFunctorManagerImpl() = default;

  SVTKM_CONT
  TransformFunctorManagerImpl(const ProvidedFunctorType& functor)
    : Functor(functor)
  {
  }

  SVTKM_CONT
  auto PrepareForControl() const -> decltype(this->Functor.PrepareForControl())
  {
    return this->Functor.PrepareForControl();
  }

  template <typename Device>
  SVTKM_CONT auto PrepareForExecution(Device device) const
    -> decltype(this->Functor.PrepareForExecution(device))
  {
    return this->Functor.PrepareForExecution(device);
  }
};

template <typename ProvidedFunctorType>
struct TransformFunctorManager
  : TransformFunctorManagerImpl<
      ProvidedFunctorType,
      typename svtkm::cont::internal::IsExecutionAndControlObjectBase<ProvidedFunctorType>::type>
{
  using Superclass = TransformFunctorManagerImpl<
    ProvidedFunctorType,
    typename svtkm::cont::internal::IsExecutionAndControlObjectBase<ProvidedFunctorType>::type>;
  using FunctorType = typename Superclass::FunctorType;

  SVTKM_CONT TransformFunctorManager() = default;

  SVTKM_CONT TransformFunctorManager(const TransformFunctorManager&) = default;

  SVTKM_CONT TransformFunctorManager(const ProvidedFunctorType& functor)
    : Superclass(functor)
  {
  }

  template <typename ValueType>
  using TransformedValueType = decltype(std::declval<FunctorType>()(ValueType{}));
};

template <typename ArrayHandleType,
          typename FunctorType,
          typename InverseFunctorType = NullFunctorType>
struct SVTKM_ALWAYS_EXPORT StorageTagTransform
{
  using FunctorManager = TransformFunctorManager<FunctorType>;
  using ValueType =
    typename FunctorManager::template TransformedValueType<typename ArrayHandleType::ValueType>;
};

template <typename ArrayHandleType, typename FunctorType>
class Storage<typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType,
              StorageTagTransform<ArrayHandleType, FunctorType>>
{
  using FunctorManager = TransformFunctorManager<FunctorType>;

public:
  using ValueType = typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType;

  using PortalConstType =
    svtkm::exec::internal::ArrayPortalTransform<ValueType,
                                               typename ArrayHandleType::PortalConstControl,
                                               typename FunctorManager::FunctorType>;

  // Note that this array is read only, so you really should only be getting the const
  // version of the portal. If you actually try to write to this portal, you will
  // get an error.
  using PortalType = PortalConstType;

  SVTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  SVTKM_CONT
  Storage(const ArrayHandleType& array, const FunctorType& functor = FunctorType())
    : Array(array)
    , Functor(functor)
    , Valid(true)
  {
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    throw svtkm::cont::ErrorBadType(
      "ArrayHandleTransform is read only. Cannot get writable portal.");
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->Valid);
    svtkm::cont::ScopedRuntimeDeviceTracker trackerScope(svtkm::cont::DeviceAdapterTagSerial{});
    return PortalConstType(this->Array.GetPortalConstControl(), this->Functor.PrepareForControl());
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->Array.GetNumberOfValues();
  }

  SVTKM_CONT
  void Allocate(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("ArrayHandleTransform is read only. It cannot be allocated.");
  }

  SVTKM_CONT
  void Shrink(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("ArrayHandleTransform is read only. It cannot shrink.");
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the delegate array, which may be used elsewhere. Should the behavior
    // be different?
  }

  SVTKM_CONT
  const ArrayHandleType& GetArray() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->Array;
  }

  SVTKM_CONT
  const FunctorManager& GetFunctor() const { return this->Functor; }

private:
  ArrayHandleType Array;
  FunctorManager Functor;
  bool Valid;
};

template <typename ArrayHandleType, typename FunctorType, typename InverseFunctorType>
class Storage<
  typename StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::ValueType,
  StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>>
{
  using FunctorManager = TransformFunctorManager<FunctorType>;
  using InverseFunctorManager = TransformFunctorManager<InverseFunctorType>;

public:
  using ValueType =
    typename StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::ValueType;

  using PortalType =
    svtkm::exec::internal::ArrayPortalTransform<ValueType,
                                               typename ArrayHandleType::PortalControl,
                                               typename FunctorManager::FunctorType,
                                               typename InverseFunctorManager::FunctorType>;
  using PortalConstType =
    svtkm::exec::internal::ArrayPortalTransform<ValueType,
                                               typename ArrayHandleType::PortalConstControl,
                                               typename FunctorManager::FunctorType,
                                               typename InverseFunctorManager::FunctorType>;

  SVTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  SVTKM_CONT
  Storage(const ArrayHandleType& array,
          const FunctorType& functor = FunctorType(),
          const InverseFunctorType& inverseFunctor = InverseFunctorType())
    : Array(array)
    , Functor(functor)
    , InverseFunctor(inverseFunctor)
    , Valid(true)
  {
  }

  SVTKM_CONT
  PortalType GetPortal()
  {
    SVTKM_ASSERT(this->Valid);
    svtkm::cont::ScopedRuntimeDeviceTracker trackerScope(svtkm::cont::DeviceAdapterTagSerial{});
    return PortalType(this->Array.GetPortalControl(),
                      this->Functor.PrepareForControl(),
                      this->InverseFunctor.PrepareForControl());
  }

  SVTKM_CONT
  PortalConstType GetPortalConst() const
  {
    SVTKM_ASSERT(this->Valid);
    svtkm::cont::ScopedRuntimeDeviceTracker trackerScope(svtkm::cont::DeviceAdapterTagSerial{});
    return PortalConstType(this->Array.GetPortalConstControl(),
                           this->Functor.PrepareForControl(),
                           this->InverseFunctor.PrepareForControl());
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->Array.GetNumberOfValues();
  }

  SVTKM_CONT
  void Allocate(svtkm::Id numberOfValues)
  {
    this->Array.Allocate(numberOfValues);
    this->Valid = true;
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues) { this->Array.Shrink(numberOfValues); }

  SVTKM_CONT
  void ReleaseResources()
  {
    this->Array.ReleaseResources();
    this->Valid = false;
  }

  SVTKM_CONT
  const ArrayHandleType& GetArray() const
  {
    SVTKM_ASSERT(this->Valid);
    return this->Array;
  }

  SVTKM_CONT
  const FunctorManager& GetFunctor() const { return this->Functor; }

  SVTKM_CONT
  const InverseFunctorManager& GetInverseFunctor() const { return this->InverseFunctor; }

private:
  ArrayHandleType Array;
  FunctorManager Functor;
  InverseFunctorManager InverseFunctor;
  bool Valid;
};

template <typename ArrayHandleType, typename FunctorType, typename Device>
class ArrayTransfer<typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType,
                    StorageTagTransform<ArrayHandleType, FunctorType>,
                    Device>
{
  using StorageTag = StorageTagTransform<ArrayHandleType, FunctorType>;
  using FunctorManager = TransformFunctorManager<FunctorType>;

public:
  using ValueType = typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType;
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  //meant to be an invalid writeable execution portal
  using PortalExecution = typename StorageType::PortalType;
  using PortalConstExecution = svtkm::exec::internal::ArrayPortalTransform<
    ValueType,
    typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst,
    typename FunctorManager::FunctorType>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
    , Functor(storage->GetFunctor())
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->Array.PrepareForInput(Device()),
                                this->Functor.PrepareForExecution(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool& svtkmNotUsed(updateData))
  {
    throw svtkm::cont::ErrorBadType("ArrayHandleTransform read only. "
                                   "Cannot be used for in-place operations.");
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("ArrayHandleTransform read only. Cannot be used as output.");
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    throw svtkm::cont::ErrorInternal(
      "ArrayHandleTransform read only. "
      "There should be no occurrence of the ArrayHandle trying to pull "
      "data from the execution environment.");
  }

  SVTKM_CONT
  void Shrink(svtkm::Id svtkmNotUsed(numberOfValues))
  {
    throw svtkm::cont::ErrorBadType("ArrayHandleTransform read only. Cannot shrink.");
  }

  SVTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }

private:
  ArrayHandleType Array;
  FunctorManager Functor;
};

template <typename ArrayHandleType,
          typename FunctorType,
          typename InverseFunctorType,
          typename Device>
class ArrayTransfer<
  typename StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::ValueType,
  StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>,
  Device>
{
  using StorageTag = StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>;
  using FunctorManager = TransformFunctorManager<FunctorType>;
  using InverseFunctorManager = TransformFunctorManager<InverseFunctorType>;

public:
  using ValueType = typename StorageTagTransform<ArrayHandleType, FunctorType>::ValueType;
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = svtkm::exec::internal::ArrayPortalTransform<
    ValueType,
    typename ArrayHandleType::template ExecutionTypes<Device>::Portal,
    typename FunctorManager::FunctorType,
    typename InverseFunctorManager::FunctorType>;
  using PortalConstExecution = svtkm::exec::internal::ArrayPortalTransform<
    ValueType,
    typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst,
    typename FunctorManager::FunctorType,
    typename InverseFunctorManager::FunctorType>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetArray())
    , Functor(storage->GetFunctor())
    , InverseFunctor(storage->GetInverseFunctor())
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->Array.PrepareForInput(Device()),
                                this->Functor.PrepareForExecution(Device()),
                                this->InverseFunctor.PrepareForExecution(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool& svtkmNotUsed(updateData))
  {
    return PortalExecution(this->Array.PrepareForInPlace(Device()),
                           this->Functor.PrepareForExecution(Device()),
                           this->InverseFunctor.PrepareForExecution(Device()));
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numberOfValues)
  {
    return PortalExecution(this->Array.PrepareForOutput(numberOfValues, Device()),
                           this->Functor.PrepareForExecution(Device()),
                           this->InverseFunctor.PrepareForExecution(Device()));
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* svtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handle should automatically retrieve the output data as necessary.
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues) { this->Array.Shrink(numberOfValues); }

  SVTKM_CONT
  void ReleaseResources() { this->Array.ReleaseResourcesExecution(); }

private:
  ArrayHandleType Array;
  FunctorManager Functor;
  InverseFunctorManager InverseFunctor;
};

} // namespace internal

/// \brief Implicitly transform values of one array to another with a functor.
///
/// ArrayHandleTransforms is a specialization of ArrayHandle. It takes a
/// delegate array handle and makes a new handle that calls a given unary
/// functor with the element at a given index and returns the result of the
/// functor as the value of this array at that position. This transformation is
/// done on demand. That is, rather than make a new copy of the array with new
/// values, the transformation is done as values are read from the array. Thus,
/// the functor operator should work in both the control and execution
/// environments.
///
template <typename ArrayHandleType,
          typename FunctorType,
          typename InverseFunctorType = internal::NullFunctorType>
class ArrayHandleTransform;

template <typename ArrayHandleType, typename FunctorType>
class ArrayHandleTransform<ArrayHandleType, FunctorType, internal::NullFunctorType>
  : public svtkm::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType>::ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType>>
{
  // If the following line gives a compile error, then the ArrayHandleType
  // template argument is not a valid ArrayHandle type.
  SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleTransform,
    (ArrayHandleTransform<ArrayHandleType, FunctorType>),
    (svtkm::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType>::ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType>>));

private:
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  SVTKM_CONT
  ArrayHandleTransform(const ArrayHandleType& handle, const FunctorType& functor = FunctorType())
    : Superclass(StorageType(handle, functor))
  {
  }
};

/// make_ArrayHandleTransform is convenience function to generate an
/// ArrayHandleTransform.  It takes in an ArrayHandle and a functor
/// to apply to each element of the Handle.
template <typename HandleType, typename FunctorType>
SVTKM_CONT svtkm::cont::ArrayHandleTransform<HandleType, FunctorType> make_ArrayHandleTransform(
  HandleType handle,
  FunctorType functor)
{
  return ArrayHandleTransform<HandleType, FunctorType>(handle, functor);
}

// ArrayHandleTransform with inverse functors enabled (no need to subclass from
// ArrayHandleTransform without inverse functors: nothing to inherit).
template <typename ArrayHandleType, typename FunctorType, typename InverseFunctorType>
class ArrayHandleTransform
  : public svtkm::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::
        ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>>
{
  SVTKM_IS_ARRAY_HANDLE(ArrayHandleType);

public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleTransform,
    (ArrayHandleTransform<ArrayHandleType, FunctorType, InverseFunctorType>),
    (svtkm::cont::ArrayHandle<
      typename internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>::
        ValueType,
      internal::StorageTagTransform<ArrayHandleType, FunctorType, InverseFunctorType>>));

private:
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  ArrayHandleTransform(const ArrayHandleType& handle,
                       const FunctorType& functor = FunctorType(),
                       const InverseFunctorType& inverseFunctor = InverseFunctorType())
    : Superclass(StorageType(handle, functor, inverseFunctor))
  {
  }

  /// Implemented so that it is defined exclusively in the control environment.
  /// If there is a separate device for the execution environment (for example,
  /// with CUDA), then the automatically generated destructor could be
  /// created for all devices, and it would not be valid for all devices.
  ///
  ~ArrayHandleTransform() {}
};

template <typename HandleType, typename FunctorType, typename InverseFunctorType>
SVTKM_CONT svtkm::cont::ArrayHandleTransform<HandleType, FunctorType, InverseFunctorType>
make_ArrayHandleTransform(HandleType handle, FunctorType functor, InverseFunctorType inverseFunctor)
{
  return ArrayHandleTransform<HandleType, FunctorType, InverseFunctorType>(
    handle, functor, inverseFunctor);
}
}

} // namespace svtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace svtkm
{
namespace cont
{

template <typename AH, typename Functor, typename InvFunctor>
struct SerializableTypeString<svtkm::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Transform<" + SerializableTypeString<AH>::Get() + "," +
      SerializableTypeString<Functor>::Get() + "," + SerializableTypeString<InvFunctor>::Get() +
      ">";
    return name;
  }
};

template <typename AH, typename Functor>
struct SerializableTypeString<svtkm::cont::ArrayHandleTransform<AH, Functor>>
{
  static SVTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Transform<" + SerializableTypeString<AH>::Get() + "," +
      SerializableTypeString<Functor>::Get() + ">";
    return name;
  }
};

template <typename AH, typename Functor, typename InvFunctor>
struct SerializableTypeString<svtkm::cont::ArrayHandle<
  typename svtkm::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>::ValueType,
  svtkm::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>>>
  : SerializableTypeString<svtkm::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
};
}
} // svtkm::cont

namespace mangled_diy_namespace
{

template <typename AH, typename Functor>
struct Serialization<svtkm::cont::ArrayHandleTransform<AH, Functor>>
{
private:
  using Type = svtkm::cont::ArrayHandleTransform<AH, Functor>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    svtkmdiy::save(bb, storage.GetArray());
    svtkmdiy::save(bb, storage.GetFunctor());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    svtkmdiy::load(bb, array);
    Functor functor;
    svtkmdiy::load(bb, functor);
    obj = svtkm::cont::make_ArrayHandleTransform(array, functor);
  }
};

template <typename AH, typename Functor, typename InvFunctor>
struct Serialization<svtkm::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
private:
  using Type = svtkm::cont::ArrayHandleTransform<AH, Functor, InvFunctor>;
  using BaseType = svtkm::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
  static SVTKM_CONT void save(BinaryBuffer& bb, const BaseType& obj)
  {
    auto storage = obj.GetStorage();
    svtkmdiy::save(bb, storage.GetArray());
    svtkmdiy::save(bb, storage.GetFunctor());
    svtkmdiy::save(bb, storage.GetInverseFunctor());
  }

  static SVTKM_CONT void load(BinaryBuffer& bb, BaseType& obj)
  {
    AH array;
    svtkmdiy::load(bb, array);
    Functor functor;
    svtkmdiy::load(bb, functor);
    InvFunctor invFunctor;
    svtkmdiy::load(bb, invFunctor);
    obj = svtkm::cont::make_ArrayHandleTransform(array, functor, invFunctor);
  }
};

template <typename AH, typename Functor, typename InvFunctor>
struct Serialization<svtkm::cont::ArrayHandle<
  typename svtkm::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>::ValueType,
  svtkm::cont::internal::StorageTagTransform<AH, Functor, InvFunctor>>>
  : Serialization<svtkm::cont::ArrayHandleTransform<AH, Functor, InvFunctor>>
{
};

} // diy
/// @endcond SERIALIZATION

#endif //svtk_m_cont_ArrayHandleTransform_h
