//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define svtk_m_cont_tbb_internal_ArrayManagerExecutionTBB_cxx

#include <svtkm/cont/tbb/internal/ArrayManagerExecutionTBB.h>

namespace svtkm
{
namespace cont
{

SVTKM_INSTANTIATE_ARRAYHANDLES_FOR_DEVICE_ADAPTER(DeviceAdapterTagTBB)
}
} // end svtkm::cont
