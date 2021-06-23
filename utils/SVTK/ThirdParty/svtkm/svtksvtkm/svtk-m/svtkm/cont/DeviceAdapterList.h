//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_DeviceAdapterList_h
#define svtk_m_cont_DeviceAdapterList_h

#ifndef SVTKM_DEFAULT_DEVICE_ADAPTER_LIST
#define SVTKM_DEFAULT_DEVICE_ADAPTER_LIST ::svtkm::cont::DeviceAdapterListCommon
#endif

#include <svtkm/List.h>

#include <svtkm/cont/cuda/DeviceAdapterCuda.h>
#include <svtkm/cont/openmp/DeviceAdapterOpenMP.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/cont/tbb/DeviceAdapterTBB.h>

namespace svtkm
{
namespace cont
{

using DeviceAdapterListCommon = svtkm::List<svtkm::cont::DeviceAdapterTagCuda,
                                           svtkm::cont::DeviceAdapterTagTBB,
                                           svtkm::cont::DeviceAdapterTagOpenMP,
                                           svtkm::cont::DeviceAdapterTagSerial>;
}
} // namespace svtkm::cont

#endif //svtk_m_cont_DeviceAdapterList_h
