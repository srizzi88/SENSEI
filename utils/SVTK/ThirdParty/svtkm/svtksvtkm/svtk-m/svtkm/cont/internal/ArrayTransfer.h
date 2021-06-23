//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_ArrayTransfer_h
#define svtk_m_cont_internal_ArrayTransfer_h

#include <svtkm/cont/Storage.h>
#include <svtkm/cont/internal/ArrayManagerExecution.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

/// \brief Class that manages the transfer of data between control and execution.
///
/// This templated class provides a mechanism (used by the ArrayHandle) to
/// transfer data from the control environment to the execution environment and
/// back. The interface for ArrayTransfer is nearly identical to that of
/// ArrayManagerExecution and the default implementation simply delegates all
/// calls to that class.
///
/// The primary motivation for having a separate class is that the
/// ArrayManagerExecution is meant to be specialized for each device adapter
/// whereas the ArrayTransfer is meant to be specialized for each storage type
/// (or specific combination of storage and device adapter). Thus, transfers
/// for most storage tyeps will be delegated through the ArrayManagerExecution,
/// but some storage types, like implicit storage, will be specialized to
/// transfer through a different path.
///
template <typename T, class StorageTag, class DeviceAdapterTag>
class ArrayTransfer
{
private:
  using StorageType = svtkm::cont::internal::Storage<T, StorageTag>;
  using ArrayManagerType =
    svtkm::cont::internal::ArrayManagerExecution<T, StorageTag, DeviceAdapterTag>;

public:
  /// The type of value held in the array (svtkm::FloatDefault, svtkm::Vec, etc.)
  ///
  using ValueType = T;

  /// An array portal that can be used in the control environment.
  ///
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  /// An array portal that can be used in the execution environment.
  ///
  using PortalExecution = typename ArrayManagerType::PortalType;
  using PortalConstExecution = typename ArrayManagerType::PortalConstType;

  SVTKM_CONT
  ArrayTransfer(StorageType* storage)
    : ArrayManager(storage)
  {
  }

  /// Returns the number of values stored in the array.  Results are undefined
  /// if data has not been loaded or allocated.
  ///
  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const { return this->ArrayManager.GetNumberOfValues(); }

  /// Prepares the data for use as input in the execution environment. If the
  /// flag \c updateData is true, then data is transferred to the execution
  /// environment. Otherwise, this transfer is (or may be) skipped.
  ///
  /// Returns a constant array portal valid in the execution environment.
  ///
  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool updateData)
  {
    return this->ArrayManager.PrepareForInput(updateData);
  }

  /// Prepares the data for use as both input and output in the execution
  /// environment. If the flag \c updateData is true, then data is transferred
  /// to the execution environment. Otherwise, this transfer is (or may be)
  /// skipped.
  ///
  /// Returns a read-write array portal valid in the execution environment.
  ///
  SVTKM_CONT
  PortalExecution PrepareForInPlace(bool updateData)
  {
    return this->ArrayManager.PrepareForInPlace(updateData);
  }

  /// Allocates an array in the execution environment of the specified size. If
  /// control and execution share arrays, then this class can allocate data
  /// using the given Storage it can be used directly in the execution
  /// environment.
  ///
  /// Returns a writable array portal valid in the execution environment.
  ///
  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numberOfValues)
  {
    return this->ArrayManager.PrepareForOutput(numberOfValues);
  }

  /// Allocates data in the given Storage and copies data held in the execution
  /// environment (managed by this class) into the storage object. The
  /// reference to the storage given is the same as that passed to the
  /// constructor. If control and execution share arrays, this can be no
  /// operation. This method should only be called after PrepareForOutput is
  /// called.
  ///
  SVTKM_CONT
  void RetrieveOutputData(StorageType* storage) const
  {
    this->ArrayManager.RetrieveOutputData(storage);
  }

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  ///
  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues) { this->ArrayManager.Shrink(numberOfValues); }

  /// Frees any resources (i.e. memory) allocated for the exeuction
  /// environment, if any.
  ///
  SVTKM_CONT
  void ReleaseResources() { this->ArrayManager.ReleaseResources(); }

private:
  ArrayManagerType ArrayManager;
};
}
}
} // namespace svtkm::cont::internal

#endif //svtk_m_cont_internal_ArrayTransfer_h
