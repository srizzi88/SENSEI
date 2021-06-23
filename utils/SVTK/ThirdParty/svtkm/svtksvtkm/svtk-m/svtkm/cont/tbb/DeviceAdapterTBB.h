//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_tbb_DeviceAdapterTBB_h
#define svtk_m_cont_tbb_DeviceAdapterTBB_h

#include <svtkm/cont/tbb/internal/DeviceAdapterRuntimeDetectorTBB.h>
#include <svtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>

#ifdef SVTKM_ENABLE_TBB
#include <svtkm/cont/tbb/internal/ArrayManagerExecutionTBB.h>
#include <svtkm/cont/tbb/internal/AtomicInterfaceExecutionTBB.h>
#include <svtkm/cont/tbb/internal/DeviceAdapterAlgorithmTBB.h>
#include <svtkm/cont/tbb/internal/VirtualObjectTransferTBB.h>
#endif

#endif //svtk_m_cont_tbb_DeviceAdapterTBB_h
