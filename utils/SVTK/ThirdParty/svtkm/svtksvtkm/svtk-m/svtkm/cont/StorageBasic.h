//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_StorageBasic_h
#define svtk_m_cont_StorageBasic_h

#include <svtkm/Assert.h>
#include <svtkm/Pair.h>
#include <svtkm/Types.h>
#include <svtkm/cont/ErrorBadAllocation.h>
#include <svtkm/cont/ErrorBadValue.h>
#include <svtkm/cont/Storage.h>

#include <svtkm/cont/internal/ArrayPortalFromIterators.h>

namespace svtkm
{
namespace cont
{

/// A tag for the basic implementation of a Storage object.
struct SVTKM_ALWAYS_EXPORT StorageTagBasic
{
};

namespace internal
{

/// Function that does all of SVTK-m de-allocations for storage basic.
/// This is exists so that stolen arrays can call the correct free
/// function ( _aligned_malloc / cuda_free ).
SVTKM_CONT_EXPORT void free_memory(void* ptr);


/// Class that does all of SVTK-m allocations
/// for storage basic. This is exists so that
/// stolen arrays can call the correct free
/// function ( _aligned_malloc ) on windows
struct SVTKM_CONT_EXPORT StorageBasicAllocator
{
  void* allocate(size_t size, size_t align);

  template <typename T>
  inline void deallocate(T* p)
  {
    internal::free_memory(static_cast<void*>(p));
  }
};

/// Base class for basic storage classes. This allow us to implement
/// svtkm::cont::Storage<T, StorageTagBasic > for any T type with no overhead
/// as all heavy logic is provide by a type-agnostic API including allocations, etc.
class SVTKM_CONT_EXPORT StorageBasicBase
{
public:
  using AllocatorType = StorageBasicAllocator;
  SVTKM_CONT StorageBasicBase();

  /// A non owning view of already allocated memory
  SVTKM_CONT StorageBasicBase(const void* array, svtkm::Id size, svtkm::UInt64 sizeOfValue);

  /// Transfer the ownership of already allocated memory to SVTK-m
  SVTKM_CONT StorageBasicBase(const void* array,
                             svtkm::Id size,
                             svtkm::UInt64 sizeOfValue,
                             void (*deleteFunction)(void*));


  SVTKM_CONT ~StorageBasicBase();

  SVTKM_CONT StorageBasicBase(StorageBasicBase&& src) noexcept;
  SVTKM_CONT StorageBasicBase(const StorageBasicBase& src);
  SVTKM_CONT StorageBasicBase& operator=(StorageBasicBase&& src) noexcept;
  SVTKM_CONT StorageBasicBase& operator=(const StorageBasicBase& src);

  /// \brief Return the number of bytes allocated for this storage object(Capacity).
  ///
  ///
  SVTKM_CONT svtkm::UInt64 GetNumberOfBytes() const { return this->AllocatedByteSize; }

  /// \brief Return the number of 'T' values allocated by this storage
  SVTKM_CONT svtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  /// \brief Allocates an array with the specified number of elements.
  ///
  /// The allocation may be done on an already existing array, but can wipe out
  /// any data already in the array. This method can throw
  /// ErrorBadAllocation if the array cannot be allocated or
  /// ErrorBadValue if the allocation is not feasible (for example, the
  /// array storage is read-only).
  SVTKM_CONT void AllocateValues(svtkm::Id numberOfValues, svtkm::UInt64 sizeOfValue);

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// size of the array is changed so that it can hold \c numberOfValues values.
  /// The data in the reallocated array stays the same, but \c numberOfValues must be
  /// equal or less than the preexisting size. That is, this method can only be
  /// used to shorten the array, not lengthen.
  SVTKM_CONT void Shrink(svtkm::Id numberOfValues);

  /// \brief Frees any resources (i.e. memory) stored in this array.
  ///
  /// After calling this method GetNumberOfBytes() will return 0. The
  /// resources should also be released when the Storage class is
  /// destroyed.
  SVTKM_CONT void ReleaseResources();

  /// \brief Returns if svtkm will deallocate this memory. SVTK-m StorageBasic
  /// is designed that SVTK-m will not deallocate user passed memory, or
  /// instances that have been stolen (\c StealArray)
  SVTKM_CONT bool WillDeallocate() const { return this->DeleteFunction != nullptr; }

  /// \brief Change the Change the pointer that this class is using. Should only be used
  /// by ExecutionArrayInterface sublcasses.
  /// Note: This will release any previous allocated memory!
  SVTKM_CONT void SetBasePointer(const void* ptr,
                                svtkm::Id numberOfValues,
                                svtkm::UInt64 sizeOfValue,
                                void (*deleteFunction)(void*));

  /// Return the memory location of the first element of the array data.
  SVTKM_CONT void* GetBasePointer() const;

  SVTKM_CONT void* GetEndPointer(svtkm::Id numberOfValues, svtkm::UInt64 sizeOfValue) const;

  /// Return the memory location of the first element past the end of the
  /// array's allocated memory buffer.
  SVTKM_CONT void* GetCapacityPointer() const;

protected:
  void* Array;
  svtkm::UInt64 AllocatedByteSize;
  svtkm::Id NumberOfValues;
  void (*DeleteFunction)(void*);
};

/// A basic implementation of an Storage object.
///
/// \todo This storage does \em not construct the values within the array.
/// Thus, it is important to not use this class with any type that will fail if
/// not constructed. These are things like basic types (int, float, etc.) and
/// the SVTKm Tuple classes.  In the future it would be nice to have a compile
/// time check to enforce this.
///
template <typename ValueT>
class SVTKM_ALWAYS_EXPORT Storage<ValueT, svtkm::cont::StorageTagBasic> : public StorageBasicBase
{
public:
  using AllocatorType = svtkm::cont::internal::StorageBasicAllocator;
  using ValueType = ValueT;
  using PortalType = svtkm::cont::internal::ArrayPortalFromIterators<ValueType*>;
  using PortalConstType = svtkm::cont::internal::ArrayPortalFromIterators<const ValueType*>;

public:
  /// \brief construct storage that SVTK-m is responsible for
  SVTKM_CONT Storage();
  SVTKM_CONT Storage(const Storage<ValueT, svtkm::cont::StorageTagBasic>& src);
  SVTKM_CONT Storage(Storage<ValueT, svtkm::cont::StorageTagBasic>&& src) noexcept;

  /// \brief construct storage that SVTK-m is not responsible for
  SVTKM_CONT Storage(const ValueType* array, svtkm::Id numberOfValues);

  /// \brief construct storage that was previously allocated and now SVTK-m is
  ///  responsible for
  SVTKM_CONT Storage(const ValueType* array, svtkm::Id numberOfValues, void (*deleteFunction)(void*));

  SVTKM_CONT Storage& operator=(const Storage<ValueT, svtkm::cont::StorageTagBasic>& src);
  SVTKM_CONT Storage& operator=(Storage<ValueT, svtkm::cont::StorageTagBasic>&& src);

  SVTKM_CONT void Allocate(svtkm::Id numberOfValues);

  SVTKM_CONT PortalType GetPortal();

  SVTKM_CONT PortalConstType GetPortalConst() const;

  /// \brief Get a pointer to the underlying data structure.
  ///
  /// This method returns the pointer to the array held by this array. The
  /// memory associated with this array still belongs to the Storage (i.e.
  /// Storage will eventually deallocate the array).
  ///
  SVTKM_CONT ValueType* GetArray();

  SVTKM_CONT const ValueType* GetArray() const;

  /// \brief Transfer ownership of the underlying away from this object.
  ///
  /// This method returns the pointer and free function to the array held
  /// by this array. It then clears the internal ownership flags, thereby ensuring
  /// that the Storage will never deallocate the array or be able to reallocate it.
  /// This is helpful for taking a reference for an array created internally by
  /// SVTK-m and not having to keep a SVTK-m object around. Obviously the caller
  /// becomes responsible for destroying the memory, and should use the provided
  /// delete function.
  ///
  SVTKM_CONT svtkm::Pair<ValueType*, void (*)(void*)> StealArray();
};

} // namespace internal
}
} // namespace svtkm::cont

#ifndef svtkm_cont_StorageBasic_cxx
namespace svtkm
{
namespace cont
{
namespace internal
{

/// \cond
/// Make doxygen ignore this section
#define SVTKM_STORAGE_EXPORT(Type)                                                                  \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT Storage<Type, StorageTagBasic>;                  \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT Storage<svtkm::Vec<Type, 2>, StorageTagBasic>;    \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT Storage<svtkm::Vec<Type, 3>, StorageTagBasic>;    \
  extern template class SVTKM_CONT_TEMPLATE_EXPORT Storage<svtkm::Vec<Type, 4>, StorageTagBasic>;

SVTKM_STORAGE_EXPORT(char)
SVTKM_STORAGE_EXPORT(svtkm::Int8)
SVTKM_STORAGE_EXPORT(svtkm::UInt8)
SVTKM_STORAGE_EXPORT(svtkm::Int16)
SVTKM_STORAGE_EXPORT(svtkm::UInt16)
SVTKM_STORAGE_EXPORT(svtkm::Int32)
SVTKM_STORAGE_EXPORT(svtkm::UInt32)
SVTKM_STORAGE_EXPORT(svtkm::Int64)
SVTKM_STORAGE_EXPORT(svtkm::UInt64)
SVTKM_STORAGE_EXPORT(svtkm::Float32)
SVTKM_STORAGE_EXPORT(svtkm::Float64)

#undef SVTKM_STORAGE_EXPORT
/// \endcond
}
}
}
#endif

#include <svtkm/cont/StorageBasic.hxx>

#endif //svtk_m_cont_StorageBasic_h
