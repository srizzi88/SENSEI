//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_ArrayManagerExecution_h
#define svtk_m_cont_internal_ArrayManagerExecution_h

#include <svtkm/cont/DeviceAdapterTag.h>

namespace svtkm
{
namespace cont
{
namespace internal
{

/// \brief Class that manages data in the execution environment.
///
/// This templated class must be partially specialized for each
/// DeviceAdapterTag created, which will define the implementation for that tag.
///
/// This is a class that is responsible for allocating data in the execution
/// environment and copying data back and forth between control and
/// execution. It is also expected that this class will automatically release
/// any resources in its destructor.
///
/// This class typically takes on one of two forms. If the control and
/// execution environments have separate memory spaces, then this class
/// behaves how you would expect. It allocates/deallocates arrays and copies
/// data. However, if the control and execution environments share the same
/// memory space, this class should delegate all its operations to the
/// \c Storage. The latter can probably be implemented with a
/// trivial subclass of
/// svtkm::cont::internal::ArrayManagerExecutionShareWithControl.
///
template <typename T, class StorageTag, class DeviceAdapterTag>
class ArrayManagerExecution
#ifdef SVTKM_DOXYGEN_ONLY
{
private:
  using StorageType = svtkm::cont::internal::Storage<T, StorageTag>;

public:
  /// The type of value held in the array (svtkm::FloatDefault, svtkm::Vec, etc.)
  ///
  using ValueType = T;

  /// An array portal that can be used in the execution environment to access
  /// portions of the arrays. This example defines the portal with a pointer,
  /// but any portal with methods that can be called and data that can be
  /// accessed from the execution environment can be used.
  ///
  using PortalType = svtkm::exec::internal::ArrayPortalFromIterators<ValueType*>;

  /// Const version of PortalType.  You must be able to cast PortalType to
  /// PortalConstType.
  ///
  using PortalConstType = svtkm::exec::internal::ArrayPortalFromIterators<const ValueType*>;

  /// All ArrayManagerExecution classes must have a constructor that takes a
  /// storage reference. The reference may be saved (and will remain valid
  /// throughout the life of the ArrayManagerExecution). Copying storage
  /// objects should be avoided (copy references or pointers only). The
  /// reference can also, of course, be ignored.
  ///
  SVTKM_CONT
  ArrayManagerExecution(svtkm::cont::internal::Storage<T, StorageTag>& storage);

  /// Returns the number of values stored in the array.  Results are undefined
  /// if data has not been loaded or allocated.
  ///
  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const;

  /// Prepares the data for use as input in the execution environment. If the
  /// flag \c updateData is true, then data is transferred to the execution
  /// environment. Otherwise, this transfer should be skipped.
  ///
  /// Returns a constant array portal valid in the execution environment.
  ///
  SVTKM_CONT
  PortalConstExecution PrepareForInput(bool updateData);

  /// Prepares the data for use as both input and output in the execution
  /// environment. If the flag \c updateData is true, then data is transferred
  /// to the execution environment. Otherwise, this transfer should be skipped.
  ///
  /// Returns a read-write array portal valid in the execution environment.
  ///
  SVTKM_CONT
  PortalExecution LoadDataForInPlace(bool updateData);

  /// Allocates an array in the execution environment of the specified size. If
  /// control and execution share arrays, then this class can allocate data
  /// using the given Storage it can be used directly in the execution
  /// environment.
  ///
  /// Returns a writable array portal valid in the execution environment.
  ///
  SVTKM_CONT
  PortalExecution PrepareForOutput(svtkm::Id numberOfValues);

  /// Allocates data in the given Storage and copies data held in the execution
  /// environment (managed by this class) into the storage object. The
  /// reference to the storage given is the same as that passed to the
  /// constructor. If control and execution share arrays, this can be no
  /// operation. This method should only be called after PrepareForOutput is
  /// called.
  ///
  SVTKM_CONT
  void RetrieveOutputData(svtkm::cont::internal::Storage<T, StorageTag>* storage) const;

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
  void Shrink(svtkm::Id numberOfValues);

  /// Frees any resources (i.e. memory) allocated for the exeuction
  /// environment, if any.
  ///
  SVTKM_CONT
  void ReleaseResources();
};
#else  // SVTKM_DOXGEN_ONLY
  ;
#endif // SVTKM_DOXYGEN_ONLY
}
}
} // namespace svtkm::cont::internal

#endif //svtk_m_cont_internal_ArrayManagerExecution_h
