//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_cuda_DeviceAdapterCuda_h
#define svtk_m_cont_cuda_DeviceAdapterCuda_h

#include <svtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>

#ifdef SVTKM_CUDA

//This is required to be first so that we get patches for thrust included
//in the correct order
#include <svtkm/exec/cuda/internal/ThrustPatches.h>

#include <svtkm/cont/cuda/internal/ArrayManagerExecutionCuda.h>
#include <svtkm/cont/cuda/internal/DeviceAdapterAlgorithmCuda.h>
#include <svtkm/cont/cuda/internal/VirtualObjectTransferCuda.h>
#endif

#endif //svtk_m_cont_cuda_DeviceAdapterCuda_h
