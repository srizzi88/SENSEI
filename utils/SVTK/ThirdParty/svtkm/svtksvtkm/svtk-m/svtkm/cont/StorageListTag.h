//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_StorageListTag_h
#define svtk_m_cont_StorageListTag_h

// Everything in this header file is deprecated and movded to StorageList.h.

#ifndef SVTKM_DEFAULT_STORAGE_LIST_TAG
#define SVTKM_DEFAULT_STORAGE_LIST_TAG ::svtkm::cont::detail::StorageListTagDefault
#endif

#include <svtkm/ListTag.h>

#include <svtkm/cont/StorageList.h>

namespace svtkm
{
namespace cont
{

struct SVTKM_ALWAYS_EXPORT SVTKM_DEPRECATED(
  1.6,
  "StorageListTagBasic replaced by StorageListBasic. "
  "Note that the new StorageListBasic cannot be subclassed.") StorageListTagBasic
  : svtkm::internal::ListAsListTag<StorageListBasic>
{
};

struct SVTKM_ALWAYS_EXPORT SVTKM_DEPRECATED(
  1.6,
  "StorageListTagSupported replaced by StorageListSupported. "
  "Note that the new StorageListSupported cannot be subclassed.") StorageListTagSupported
  : svtkm::internal::ListAsListTag<StorageListSupported>
{
};

namespace detail
{

struct SVTKM_ALWAYS_EXPORT SVTKM_DEPRECATED(
  1.6,
  "SVTKM_DEFAULT_STORAGE_LIST_TAG replaced by SVTKM_DEFAULT_STORAGE_LIST. "
  "Note that the new SVTKM_DEFAULT_STORAGE_LIST cannot be subclassed.") StorageListTagDefault
  : svtkm::internal::ListAsListTag<SVTKM_DEFAULT_STORAGE_LIST>
{
};

} // namespace detail
}
} // namespace svtkm::cont

#endif //svtk_m_cont_StorageListTag_h
