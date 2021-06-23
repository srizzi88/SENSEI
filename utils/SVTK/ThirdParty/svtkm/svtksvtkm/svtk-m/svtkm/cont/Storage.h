//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_Storage_h
#define svtk_m_cont_Storage_h

#define SVTKM_STORAGE_ERROR -2
#define SVTKM_STORAGE_UNDEFINED -1
#define SVTKM_STORAGE_BASIC 1

#ifndef SVTKM_STORAGE
#define SVTKM_STORAGE SVTKM_STORAGE_BASIC
#endif

#include <svtkm/StaticAssert.h>

#include <svtkm/cont/svtkm_cont_export.h>
#include <svtkm/internal/ExportMacros.h>

namespace svtkm
{
namespace cont
{

#ifdef SVTKM_DOXYGEN_ONLY
/// \brief A tag specifying client memory allocation.
///
/// A Storage tag specifies how an ArrayHandle allocates and frees memory. The
/// tag StorageTag___ does not actually exist. Rather, this documentation is
/// provided to describe how array storage objects are specified. Loading the
/// svtkm/cont/Storage.h header will set a default array storage. You can
/// specify the default storage by first setting the SVTKM_STORAGE macro.
/// Currently it can only be set to SVTKM_STORAGE_BASIC.
///
/// User code external to SVTK-m is free to make its own StorageTag. This is a
/// good way to get SVTK-m to read data directly in and out of arrays from other
/// libraries. However, care should be taken when creating a Storage. One
/// particular problem that is likely is a storage that "constructs" all the
/// items in the array. If done incorrectly, then memory of the array can be
/// incorrectly bound to the wrong processor. If you do provide your own
/// StorageTag, please be diligent in comparing its performance to the
/// StorageTagBasic.
///
/// To implement your own StorageTag, you first must create a tag class (an
/// empty struct) defining your tag (i.e. struct SVTKM_ALWAYS_EXPORT StorageTagMyAlloc { };). Then
/// provide a partial template specialization of svtkm::cont::internal::Storage
/// for your new tag.
///
struct SVTKM_ALWAYS_EXPORT StorageTag___
{
};
#endif // SVTKM_DOXYGEN_ONLY

namespace internal
{

struct UndefinedStorage
{
};

namespace detail
{

// This class should never be used. It is used as a placeholder for undefined
// Storage objects. If you get a compiler error involving this object, then it
// probably comes from trying to use an ArrayHandle with bad template
// arguments.
template <typename T>
struct UndefinedArrayPortal
{
  SVTKM_STATIC_ASSERT(sizeof(T) == static_cast<size_t>(-1));
};

} // namespace detail

/// This templated class must be partially specialized for each StorageTag
/// created, which will define the implementation for that tag.
///
template <typename T, class StorageTag>
class Storage
#ifndef SVTKM_DOXYGEN_ONLY
  : public svtkm::cont::internal::UndefinedStorage
{
public:
  using PortalType = svtkm::cont::internal::detail::UndefinedArrayPortal<T>;
  using PortalConstType = svtkm::cont::internal::detail::UndefinedArrayPortal<T>;
};
#else  //SVTKM_DOXYGEN_ONLY
{
public:
  /// The type of each item in the array.
  ///
  using ValueType = T;

  /// \brief The type of portal objects for the array.
  ///
  /// The actual portal object can take any form. This is a simple example of a
  /// portal to a C array.
  ///
  using PortalType = ::svtkm::cont::internal::ArrayPortalFromIterators<ValueType*>;

  /// \brief The type of portal objects (const version) for the array.
  ///
  /// The actual portal object can take any form. This is a simple example of a
  /// portal to a C array.
  ///
  using PortalConstType = ::svtkm::cont::internal::ArrayPortalFromIterators<const ValueType*>;

  SVTKM_CONT Storage(const Storage& src);
  SVTKM_CONT Storage(Storage&& src) noexcept;
  SVTKM_CONT Storage& operator=(const Storage& src);
  SVTKM_CONT Storage& operator=(Storage&& src);

  /// Returns a portal to the array.
  ///
  SVTKM_CONT
  PortalType GetPortal();

  /// Returns a portal to the array with immutable values.
  ///
  SVTKM_CONT
  PortalConstType GetPortalConst() const;

  /// Returns the number of entries allocated in the array.
  SVTKM_CONT
  svtkm::Id GetNumberOfValues() const;

  /// \brief Allocates an array large enough to hold the given number of values.
  ///
  /// The allocation may be done on an already existing array, but can wipe out
  /// any data already in the array. This method can throw
  /// ErrorBadAllocation if the array cannot be allocated or
  /// ErrorBadValue if the allocation is not feasible (for example, the
  /// array storage is read-only).
  ///
  SVTKM_CONT
  void Allocate(svtkm::Id numberOfValues);

  /// \brief Reduces the size of the array without changing its values.
  ///
  /// This method allows you to resize the array without reallocating it. The
  /// number of entries in the array is changed to \c numberOfValues. The data
  /// in the array (from indices 0 to \c numberOfValues - 1) are the same, but
  /// \c numberOfValues must be equal or less than the preexisting size
  /// (returned from GetNumberOfValues). That is, this method can only be used
  /// to shorten the array, not lengthen.
  SVTKM_CONT
  void Shrink(svtkm::Id numberOfValues);

  /// \brief Frees any resources (i.e. memory) stored in this array.
  ///
  /// After calling this method GetNumberOfValues will return 0. The
  /// resources should also be released when the Storage class is
  /// destroyed.
  SVTKM_CONT
  void ReleaseResources();
};
#endif // SVTKM_DOXYGEN_ONLY

} // namespace internal
}
} // namespace svtkm::cont

// This is put at the bottom of the header so that the Storage template is
// declared before any implementations are called.

#if SVTKM_STORAGE == SVTKM_STORAGE_BASIC

#include <svtkm/cont/StorageBasic.h>
#define SVTKM_DEFAULT_STORAGE_TAG ::svtkm::cont::StorageTagBasic

#elif SVTKM_STORAGE == SVTKM_STORAGE_ERROR

#include <svtkm/cont/internal/StorageError.h>
#define SVTKM_DEFAULT_STORAGE_TAG ::svtkm::cont::internal::StorageTagError

#elif (SVTKM_STORAGE == SVTKM_STORAGE_UNDEFINED) || !defined(SVTKM_STORAGE)

#ifndef SVTKM_DEFAULT_STORAGE_TAG
#warning If array storage is undefined, SVTKM_DEFAULT_STORAGE_TAG must be defined.
#endif

#endif

#endif //svtk_m_cont_Storage_h
