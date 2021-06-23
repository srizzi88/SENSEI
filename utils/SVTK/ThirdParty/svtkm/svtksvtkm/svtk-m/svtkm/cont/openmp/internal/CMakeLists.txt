##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

set(headers
  ArrayManagerExecutionOpenMP.h
  DeviceAdapterAlgorithmOpenMP.h
  DeviceAdapterRuntimeDetectorOpenMP.h
  DeviceAdapterTagOpenMP.h
  AtomicInterfaceExecutionOpenMP.h
  ExecutionArrayInterfaceBasicOpenMP.h
  FunctorsOpenMP.h
  ParallelQuickSortOpenMP.h
  ParallelRadixSortOpenMP.h
  ParallelScanOpenMP.h
  ParallelSortOpenMP.h
  VirtualObjectTransferOpenMP.h
  )

svtkm_declare_headers(${headers})

#These sources need to always be built
target_sources(svtkm_cont PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterRuntimeDetectorOpenMP.cxx
  )

#-----------------------------------------------------------------------------
if (TARGET svtkm::openmp)
  target_sources(svtkm_cont PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ArrayManagerExecutionOpenMP.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/DeviceAdapterAlgorithmOpenMP.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ExecutionArrayInterfaceBasicOpenMP.cxx
    ${CMAKE_CURRENT_SOURCE_DIR}/ParallelRadixSortOpenMP.cxx
    )
endif()
