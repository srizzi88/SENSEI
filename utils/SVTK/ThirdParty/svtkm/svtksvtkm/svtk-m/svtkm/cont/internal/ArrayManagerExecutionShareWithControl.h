//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_ArrayManagerExecutionShareWithControl_h
#define svtk_m_cont_internal_ArrayManagerExecutionShareWithControl_h

#include <svtkm/Assert.h>
#include <svtkm/Types.h>

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/Storage.h>

#include <algorithm>

namespace svtkm
{
namespace cont
{
namespace internal
{

/// \c ArrayManagerExecutionShareWithControl provides an implementation for a
/// \c ArrayManagerExecution class for a device adapter when the execution
/// and control environments share memory. This class basically defers all its
/// calls to a \c Storage class and uses the array allocated there.
///
template <typename T, class StorageTag>
class ArrayManagerExecutionShareWithControl
{
public:
  using ValueType = T;
  using StorageType = svtkm::cont::internal::Storage<ValueType, StorageTag>;
  using PortalType = typename StorageType::PortalType;
  using PortalConstType = typename StorageType::PortalConstType;

  SVTKM_CONT
  ArrayManagerExecutionShareWithControl(StorageType* storage)
    : Storage(storage)
  {
  }

  /// Returns the size of the storage.
  ///
  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  /// Returns the constant portal from the storage.
  ///
  SVTKM_CONT
  PortalConstType PrepareForInput(bool svtkmNotUsed(uploadData)) const
  {
    return this->Storage->GetPortalConst();
  }

  /// Returns the read-write portal from the storage.
  ///
  SVTKM_CONT
  PortalType PrepareForInPlace(bool svtkmNotUsed(uploadData)) { return this->Storage->GetPortal(); }

  /// Allocates data in the storage and return the portal to that.
  ///
  SVTKM_CONT
  PortalType PrepareForOutput(svtkm::Id numberOfValues)
  {
    this->Storage->Allocate(numberOfValues);
    return this->Storage->GetPortal();
  }

  /// This method is a no-op (except for a few checks). Any data written to
  /// this class's portals should already be written to the given \c
  /// controlArray (under correct operation).
  ///
  SVTKM_CONT
  void RetrieveOutputData(StorageType* storage) const
  {
    (void)storage;
    SVTKM_ASSERT(storage == this->Storage);
  }

  /// Shrinks the storage.
  ///
  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues) { this->Storage->Shrink(numberOfValues); }

  /// A no-op.
  ///
  SVTKM_CONT
  void ReleaseResources() {}

private:
  ArrayManagerExecutionShareWithControl(ArrayManagerExecutionShareWithControl<T, StorageTag>&) =
    delete;
  void operator=(ArrayManagerExecutionShareWithControl<T, StorageTag>&) = delete;

  StorageType* Storage;
};

// Specializations for basic storage:
template <typename T>
struct ExecutionPortalFactoryBasicShareWithControl
{
  using ValueType = T;
  using PortalType = ArrayPortalFromIterators<ValueType*>;
  using PortalConstType = ArrayPortalFromIterators<const ValueType*>;

  SVTKM_CONT
  static PortalType CreatePortal(ValueType* start, ValueType* end)
  {
    return PortalType(start, end);
  }

  SVTKM_CONT
  static PortalConstType CreatePortalConst(const ValueType* start, const ValueType* end)
  {
    return PortalConstType(start, end);
  }
};

struct SVTKM_CONT_EXPORT ExecutionArrayInterfaceBasicShareWithControl
  : public ExecutionArrayInterfaceBasicBase
{
  //inherit our parents constructor
  using ExecutionArrayInterfaceBasicBase::ExecutionArrayInterfaceBasicBase;

  SVTKM_CONT void Allocate(TypelessExecutionArray& execArray,
                          svtkm::Id numberOfValues,
                          svtkm::UInt64 sizeOfValue) const final;
  SVTKM_CONT void Free(TypelessExecutionArray& execArray) const final;

  SVTKM_CONT void CopyFromControl(const void* src, void* dst, svtkm::UInt64 bytes) const final;
  SVTKM_CONT void CopyToControl(const void* src, void* dst, svtkm::UInt64 bytes) const final;

  SVTKM_CONT void UsingForRead(const void* controlPtr,
                              const void* executionPtr,
                              svtkm::UInt64 numBytes) const final;
  SVTKM_CONT void UsingForWrite(const void* controlPtr,
                               const void* executionPtr,
                               svtkm::UInt64 numBytes) const final;
  SVTKM_CONT void UsingForReadWrite(const void* controlPtr,
                                   const void* executionPtr,
                                   svtkm::UInt64 numBytes) const final;
};
}
}
} // namespace svtkm::cont::internal

#endif //svtk_m_cont_internal_ArrayManagerExecutionShareWithControl_h
