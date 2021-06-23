//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_ArrayHandleDiscard_h
#define svtk_m_cont_ArrayHandleDiscard_h

#include <svtkm/TypeTraits.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/internal/Unreachable.h>

#include <type_traits>

namespace svtkm
{
namespace exec
{
namespace internal
{

/// \brief An output-only array portal with no storage. All written values are
/// discarded.
template <typename ValueType_>
class ArrayPortalDiscard
{
public:
  using ValueType = ValueType_;

  SVTKM_SUPPRESS_EXEC_WARNINGS
  SVTKM_EXEC_CONT
  ArrayPortalDiscard()
    : NumberOfValues(0)
  {
  } // needs to be host and device so that cuda can create lvalue of these

  SVTKM_CONT
  explicit ArrayPortalDiscard(svtkm::Id numValues)
    : NumberOfValues(numValues)
  {
  }

  /// Copy constructor for any other ArrayPortalDiscard with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template <class OtherV>
  SVTKM_CONT ArrayPortalDiscard(const ArrayPortalDiscard<OtherV>& src)
    : NumberOfValues(src.NumberOfValues)
  {
  }

  SVTKM_EXEC_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  ValueType Get(svtkm::Id) const
  {
    SVTKM_UNREACHABLE("Cannot read from ArrayHandleDiscard.");
    return svtkm::TypeTraits<ValueType>::ZeroInitialization();
  }

  SVTKM_EXEC
  void Set(svtkm::Id index, const ValueType&) const
  {
    SVTKM_ASSERT(index < this->GetNumberOfValues());
    (void)index;
    // no-op
  }

private:
  svtkm::Id NumberOfValues;
};

} // end namespace internal
} // end namespace exec

namespace cont
{

namespace internal
{

struct SVTKM_ALWAYS_EXPORT StorageTagDiscard
{
};

template <typename ValueType_>
class Storage<ValueType_, StorageTagDiscard>
{
public:
  using ValueType = ValueType_;
  using PortalType = svtkm::exec::internal::ArrayPortalDiscard<ValueType>;
  using PortalConstType = svtkm::exec::internal::ArrayPortalDiscard<ValueType>;

  SVTKM_CONT
  Storage() {}

  SVTKM_CONT
  PortalType GetPortal() { return PortalType(this->NumberOfValues); }

  SVTKM_CONT
  PortalConstType GetPortalConst() { return PortalConstType(this->NumberOfValues); }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  SVTKM_CONT
  void Allocate(svtkm::Id numValues) { this->NumberOfValues = numValues; }

  SVTKM_CONT
  void Shrink(svtkm::Id numValues) { this->NumberOfValues = numValues; }

  SVTKM_CONT
  void ReleaseResources() { this->NumberOfValues = 0; }

private:
  svtkm::Id NumberOfValues;
};

template <typename ValueType_, typename DeviceAdapter_>
class ArrayTransfer<ValueType_, StorageTagDiscard, DeviceAdapter_>
{
  using StorageTag = StorageTagDiscard;
  using StorageType = Storage<ValueType_, StorageTag>;

public:
  using ValueType = ValueType_;
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;
  using PortalExecution = svtkm::exec::internal::ArrayPortalDiscard<ValueType>;
  using PortalConstExecution = svtkm::exec::internal::ArrayPortalDiscard<ValueType>;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Internal(storage)
  {
  }

  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const
  {
    SVTKM_ASSERT(this->Internal != nullptr);
    return this->Internal->GetNumberOfValues();
  }

  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool svtkmNotUsed(updateData))
  {
    throw svtkm::cont::ErrorBadValue("Input access not supported: "
                                    "Cannot read from an ArrayHandleDiscard.");
  }

  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool svtkmNotUsed(updateData))
  {
    throw svtkm::cont::ErrorBadValue("InPlace access not supported: "
                                    "Cannot read from an ArrayHandleDiscard.");
  }

  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numValues)
  {
    SVTKM_ASSERT(this->Internal != nullptr);
    this->Internal->Allocate(numValues);
    return PortalConstExecution(this->Internal->GetNumberOfValues());
  }

  SVTKM_CONT
  void RetrieveOutputData(StorageType* storage) const
  {
    SVTKM_ASSERT(storage == this->Internal);
    (void)storage;
    // no-op
  }

  SVTKM_CONT
  void Shrink(svtkm::Id numValues)
  {
    SVTKM_ASSERT(this->Internal != nullptr);
    this->Internal->Shrink(numValues);
  }

  SVTKM_CONT
  void ReleaseResources()
  {
    SVTKM_ASSERT(this->Internal != nullptr);
    this->Internal->ReleaseResources();
  }

private:
  StorageType* Internal;
};

template <typename ValueType_>
struct ArrayHandleDiscardTraits
{
  using ValueType = ValueType_;
  using StorageTag = StorageTagDiscard;
  using Superclass = svtkm::cont::ArrayHandle<ValueType, StorageTag>;
};

} // end namespace internal

/// ArrayHandleDiscard is a write-only array that discards all data written to
/// it. This can be used to save memory when a filter provides optional outputs
/// that are not needed.
template <typename ValueType_>
class ArrayHandleDiscard : public internal::ArrayHandleDiscardTraits<ValueType_>::Superclass
{
public:
  SVTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleDiscard,
                             (ArrayHandleDiscard<ValueType_>),
                             (typename internal::ArrayHandleDiscardTraits<ValueType_>::Superclass));
};

/// Helper to determine if an ArrayHandle type is an ArrayHandleDiscard.
template <typename T>
struct IsArrayHandleDiscard : std::false_type
{
};

template <typename T>
struct IsArrayHandleDiscard<ArrayHandle<T, internal::StorageTagDiscard>> : std::true_type
{
};

} // end namespace cont
} // end namespace svtkm

#endif // svtk_m_cont_ArrayHandleDiscard_h
