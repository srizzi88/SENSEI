##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

#-----------------------------------------------------------------------------
# Build the configure file.
# need to set numerous SVTKm cmake properties to the naming convention
# that we expect for our C++ defines.

set(SVTKM_NO_ASSERT ${SVTKm_NO_ASSERT})
set(SVTKM_USE_DOUBLE_PRECISION ${SVTKm_USE_DOUBLE_PRECISION})
set(SVTKM_USE_64BIT_IDS ${SVTKm_USE_64BIT_IDS})

set(SVTKM_ENABLE_CUDA ${SVTKm_ENABLE_CUDA})
set(SVTKM_ENABLE_TBB ${SVTKm_ENABLE_TBB})
set(SVTKM_ENABLE_OPENMP ${SVTKm_ENABLE_OPENMP})
set(SVTKM_ENABLE_MPI ${SVTKm_ENABLE_MPI})

if(SVTKM_ENABLE_CUDA)
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\1" SVTKM_CUDA_VERSION_MAJOR ${CMAKE_CUDA_COMPILER_VERSION})
  string(REGEX REPLACE "([0-9]+)\\.([0-9]+).*" "\\2" SVTKM_CUDA_VERSION_MINOR ${CMAKE_CUDA_COMPILER_VERSION})
endif()

set(SVTKM_ENABLE_LOGGING ${SVTKm_ENABLE_LOGGING})

svtkm_get_kit_name(kit_name kit_dir)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Configure.h.in
  ${SVTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
  @ONLY)
svtkm_install_headers(svtkm/internal
  ${SVTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
  )

unset(SVTKM_ENABLE_MPI)
unset(SVTKM_ENABLE_TBB)
unset(SVTKM_ENABLE_CUDA)

unset(SVTKM_USE_64BIT_IDS)
unset(SVTKM_USE_DOUBLE_PRECISION)
unset(SVTKM_NO_ASSERT)


set(headers
  ArrayPortalHelpers.h
  ArrayPortalUniformPointCoordinates.h
  ArrayPortalValueReference.h
  ArrayPortalVirtual.h
  Assume.h
  brigand.hpp
  ConfigureFor32.h
  ConfigureFor64.h
  ConnectivityStructuredInternals.h
  ExportMacros.h
  FunctionInterface.h
  FunctionInterfaceDetailPost.h
  FunctionInterfaceDetailPre.h
  IndexTag.h
  IndicesExtrude.h
  Invocation.h
  Unreachable.h
  Variant.h
  VariantDetail.h
  Windows.h
  )

svtkm_declare_headers(${headers})

svtkm_pyexpander_generated_file(FunctionInterfaceDetailPre.h)
svtkm_pyexpander_generated_file(FunctionInterfaceDetailPost.h)
svtkm_pyexpander_generated_file(VariantDetail.h)

add_subdirectory(testing)

