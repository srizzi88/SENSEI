//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_cont_openmp_DeviceAdapterOpenMP_h
#define svtk_m_cont_openmp_DeviceAdapterOpenMP_h

#include <svtkm/cont/openmp/internal/DeviceAdapterRuntimeDetectorOpenMP.h>
#include <svtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>

#ifdef SVTKM_ENABLE_OPENMP
#include <svtkm/cont/openmp/internal/ArrayManagerExecutionOpenMP.h>
#include <svtkm/cont/openmp/internal/AtomicInterfaceExecutionOpenMP.h>
#include <svtkm/cont/openmp/internal/DeviceAdapterAlgorithmOpenMP.h>
#include <svtkm/cont/openmp/internal/VirtualObjectTransferOpenMP.h>
#endif

#endif //svtk_m_cont_openmp_DeviceAdapterOpenMP_h
