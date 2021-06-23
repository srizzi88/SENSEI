//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define svtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_cu

#include <svtkm/cont/cuda/internal/ArrayManagerExecutionCuda.h>

namespace svtkm
{
namespace cont
{
SVTKM_INSTANTIATE_ARRAYHANDLES_FOR_DEVICE_ADAPTER(DeviceAdapterTagCuda)
}
} // end svtkm::cont
