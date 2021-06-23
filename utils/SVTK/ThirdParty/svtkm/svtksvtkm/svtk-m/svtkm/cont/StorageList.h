//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_StorageList_h
#define svtk_m_cont_StorageList_h

#ifndef SVTKM_DEFAULT_STORAGE_LIST
#define SVTKM_DEFAULT_STORAGE_LIST ::svtkm::cont::StorageListBasic
#endif

#include <svtkm/List.h>

#include <svtkm/cont/Storage.h>
#include <svtkm/cont/StorageBasic.h>

namespace svtkm
{
namespace cont
{

using StorageListBasic = svtkm::List<svtkm::cont::StorageTagBasic>;

// If we want to compile SVTK-m with support of memory layouts other than the basic layout, then
// add the appropriate storage tags here.
using StorageListSupported = svtkm::List<svtkm::cont::StorageTagBasic>;
}
} // namespace svtkm::cont

#endif //svtk_m_cont_StorageList_h
