//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_openmp_internal_DeviceAdapterTagOpenMP_h
#define svtk_m_cont_openmp_internal_DeviceAdapterTagOpenMP_h

#include <svtkm/cont/DeviceAdapterTag.h>

#ifdef SVTKM_ENABLE_OPENMP
SVTKM_VALID_DEVICE_ADAPTER(OpenMP, SVTKM_DEVICE_ADAPTER_OPENMP)
#else
SVTKM_INVALID_DEVICE_ADAPTER(OpenMP, SVTKM_DEVICE_ADAPTER_OPENMP)
#endif

#endif // svtk_m_cont_openmp_internal_DeviceAdapterTagOpenMP_h
