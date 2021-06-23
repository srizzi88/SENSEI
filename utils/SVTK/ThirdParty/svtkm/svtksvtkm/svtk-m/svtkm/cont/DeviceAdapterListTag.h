//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_DeviceAdapterListTag_h
#define svtk_m_cont_DeviceAdapterListTag_h

// Everything in this header file is deprecated and movded to DeviceAdapterList.h.

#ifndef SVTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG
#define SVTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG ::svtkm::cont::detail::DeviceAdapterListTagDefault
#endif

#include <svtkm/List.h>

#include <svtkm/cont/DeviceAdapterList.h>

namespace svtkm
{
namespace cont
{

struct SVTKM_DEPRECATED(1.6,
                       "DeviceAdapterListTagCommon replaced by DeviceAdapterListCommon. "
                       "Note that the new DeviceAdapterListCommon cannot be subclassed.")
  DeviceAdapterListTagCommon : svtkm::internal::ListAsListTag<DeviceAdapterListCommon>
{
};

namespace detail
{

struct SVTKM_DEPRECATED(
  1.6,
  "SVTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG replaced by SVTKM_DEFAULT_DEVICE_ADAPTER_LIST. "
  "Note that the new SVTKM_DEFAULT_DEVICE_ADAPTER_LIST cannot be subclassed.")
  DeviceAdapterListTagDefault : svtkm::internal::ListAsListTag<SVTKM_DEFAULT_DEVICE_ADAPTER_LIST>
{
};

} // namespace detail
}
} // namespace svtkm::cont

#endif //svtk_m_cont_DeviceAdapterListTag_h
