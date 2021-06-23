##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
add_library(svtkm_lcl INTERFACE)

svtkm_get_kit_name(kit_name kit_dir)

# lcl needs C++11
target_compile_features(svtkm_lcl INTERFACE cxx_std_11)

target_include_directories(svtkm_lcl INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/svtkmlcl>
  $<INSTALL_INTERFACE:${SVTKm_INSTALL_INCLUDE_DIR}/svtkm/thirdparty/lcl/svtkmlcl>)

install(TARGETS svtkm_lcl
  EXPORT ${SVTKm_EXPORT_NAME})

## Install headers
if(NOT SVTKm_INSTALL_ONLY_LIBRARIES)
  install(DIRECTORY svtkmlcl
    DESTINATION ${SVTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
