##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
add_library(svtkm_optionparser INTERFACE)

svtkm_get_kit_name(kit_name kit_dir)

target_include_directories(svtkm_optionparser INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${SVTKm_INSTALL_INCLUDE_DIR}/svtkm/thirdparty/optionparser>)

install(TARGETS svtkm_optionparser
  EXPORT ${SVTKm_EXPORT_NAME})

## Install headers
if(NOT SVTKm_INSTALL_ONLY_LIBRARIES)
  install(DIRECTORY svtkmoptionparser
    DESTINATION ${SVTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
