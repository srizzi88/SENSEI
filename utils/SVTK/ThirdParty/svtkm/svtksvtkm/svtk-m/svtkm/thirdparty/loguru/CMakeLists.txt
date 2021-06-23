##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
add_library(svtkm_loguru INTERFACE)

svtkm_get_kit_name(kit_name kit_dir)

# loguru needs C++11
target_compile_features(svtkm_loguru INTERFACE cxx_std_11)

target_include_directories(svtkm_loguru INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${SVTKm_INSTALL_INCLUDE_DIR}/svtkm/thirdparty/loguru>)

target_link_libraries(svtkm_loguru INTERFACE ${CMAKE_DL_LIBS} Threads::Threads)

install(TARGETS svtkm_loguru
  EXPORT ${SVTKm_EXPORT_NAME})

## Install headers
if(NOT SVTKm_INSTALL_ONLY_LIBRARIES)
  install(DIRECTORY svtkmloguru
    DESTINATION ${SVTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
