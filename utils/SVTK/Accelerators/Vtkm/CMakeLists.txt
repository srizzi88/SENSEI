##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##  Copyright 2012 Sandia Corporation.
##  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
##  the U.S. Government retains certain rights in this software.
##
##=============================================================================

list(INSERT 0 CMAKE_MODULE_PATH
  "${SVTK_SOURCE_DIR}/ThirdParty/svtkm/svtksvtkm/svtk-m/CMake")

set(sources
  svtkmlib/ArrayConvertersReal.cxx
  svtkmlib/ArrayConvertersSigned.cxx
  svtkmlib/ArrayConvertersUnsigned.cxx)

set(headers
  svtkmFilterPolicy.h)

set(private_headers
  svtkmlib/ArrayConverters.hxx
  svtkmlib/Portals.hxx
  svtkmlib/PortalTraits.h
  svtkmDataArray.hxx)

set(impl_classes
  svtkmAverageToCells
  svtkmAverageToPoints
  svtkmCleanGrid
  svtkmClip
  svtkmContour
  svtkmCoordinateSystemTransform
  svtkmDataArray
  svtkmDataSet
  svtkmExternalFaces
  svtkmExtractVOI
  svtkmGradient
  svtkmHistogram
  svtkmImageConnectivity
  svtkmLevelOfDetail
  svtkmNDHistogram
  svtkmPointElevation
  svtkmPointTransform
  svtkmPolyDataNormals
  svtkmProbe
  svtkmThreshold
  svtkmTriangleMeshPointNormals
  svtkmWarpScalar
  svtkmWarpVector
  )

set(private_impl_classes
  svtkmlib/ArrayConverters
  svtkmlib/CellSetConverters
  svtkmlib/DataSetConverters
  svtkmlib/ImageDataConverter
  svtkmlib/ImplicitFunctionConverter
  svtkmlib/PolyDataConverter
  svtkmlib/Portals
  svtkmlib/UnstructuredGridConverter)

foreach (class IN LISTS impl_classes)
  list(APPEND sources "${class}.cxx")
  list(APPEND headers "${class}.h")
endforeach ()

foreach (class IN LISTS private_impl_classes)
  list(APPEND sources "${class}.cxx")
  list(APPEND private_headers "${class}.h")
endforeach ()


set(SVTKM_FILTER_INCLUDE_AOS ${SVTK_DISPATCH_AOS_ARRAYS})
set(SVTKM_FILTER_INCLUDE_SOA ${SVTK_DISPATCH_SOA_ARRAYS})
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtkmConfig.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtkmConfig.h"
  @ONLY)

list(APPEND headers
  "${CMAKE_CURRENT_BINARY_DIR}/svtkmConfig.h")

if (TARGET svtkm::cuda)
  enable_language(CUDA)

  # XXX(cmake#7519): Workaround for CMake which causes the c++11 flag set for
  # SVTK-m using target_compile_features to not work.
  if (CMAKE_VERSION VERSION_LESS 3.11)
    set(CMAKE_CUDA_STANDARD 11)
    set(CMAKE_CUDA_STANDARD_REQUIRED True)
  endif()

  # Temporarily suppress "has address taken but no possible call to it" warnings,
  # until we figure out its implications.
  # We are disabling all warnings as nvlink has no known way to suppress
  # individual warning types.
  string(APPEND CMAKE_CUDA_FLAGS " -Xnvlink -w")
endif ()

if(TARGET svtkm::cuda AND BUILD_SHARED_LIBS)
  message("Cuda is enabled. Module SVTK::AcceleratorsSVTKm will be forced to build as static.")
  svtk_module_add_module(SVTK::AcceleratorsSVTKm
    FORCE_STATIC
    SOURCES ${sources}
    HEADERS ${headers}
    PRIVATE_HEADERS ${private_headers})
else()
  svtk_module_add_module(SVTK::AcceleratorsSVTKm
    SOURCES ${sources}
    HEADERS ${headers}
    PRIVATE_HEADERS ${private_headers})
endif()
_svtk_module_real_target(svtkm_accel_target SVTK::AcceleratorsSVTKm)
svtkm_add_target_information(${svtkm_accel_target}
                            EXTENDS_SVTKM
                            MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES ${sources})

svtk_module_set_property(SVTK::AcceleratorsSVTKm
  PROPERTY  JOB_POOL_COMPILE
  VALUE     svtkm_pool)
svtk_module_link(SVTK::AcceleratorsSVTKm
  PRIVATE
    svtkm_worklet
    svtkm_filter)

if (TARGET svtkm::cuda)
  svtk_module_set_property(SVTK::AcceleratorsSVTKm
    PROPERTY  CUDA_SEPARABLE_COMPILATION
    VALUE     ON)
  svtk_module_compile_options(SVTK::AcceleratorsSVTKm
    PUBLIC $<$<COMPILE_LANGUAGE:CUDA>:-Xcudafe --diag_suppress=extra_semicolon>)
endif ()

# We need to install the private headers so that consumers of SVTK can write
# new SVTK-m filters and do data conversions correctly.
svtk_module_install_headers(
    FILES   ${private_headers}
    SUBDIR  "svtkmlib")

if (MSVC)
  set(msvc_warning_flags
    # Generates numerous false positives with template code about unreachable
    # code
    /wd4702
    # Generates numerous warning that implicit assignment operators can't be
    # constructed. This is understood and we don't care.
    /wd4512
    # Generates numerous warning that implicit constructors can't be
    # constructed. This is understood and we don't care.
    /wd4510
    # Generates warnings when decorated names are over 4096 characters
    # This compiler warning is deprecated and no longer occurs with VS2017+.
    # So we suppress for VS2015
    /wd4503
    )
  set(svtkm_msvc_flags)
  foreach (msvc_warning_flag IN LISTS msvc_warning_flags)
    if (TARGET svtkm::cuda)
      list(APPEND svtkm_msvc_flags
        $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=${msvc_warning_flag},${msvc_warning_flag}>)
    else ()
      list(APPEND svtkm_msvc_flags
        ${msvc_warning_flag})
    endif ()
  endforeach ()
  svtk_module_compile_options(SVTK::AcceleratorsSVTKm
    PRIVATE
      ${msvc_warning_flags})
endif ()
