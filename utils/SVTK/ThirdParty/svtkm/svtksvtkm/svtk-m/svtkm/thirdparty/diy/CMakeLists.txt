##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
add_library(svtkm_diy INTERFACE)

svtkm_get_kit_name(kit_name kit_dir)

# diy needs C++11
target_compile_features(svtkm_diy INTERFACE cxx_std_11)

# placeholder to support external DIY
set(SVTKM_USE_EXTERNAL_DIY OFF)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Configure.h.in
  ${SVTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h)

target_include_directories(svtkm_diy INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${SVTKm_INSTALL_INCLUDE_DIR}/svtkm/thirdparty/diy>)

if(SVTKm_ENABLE_MPI)
  target_link_libraries(svtkm_diy INTERFACE MPI::MPI_CXX)
endif()

install(TARGETS svtkm_diy
  EXPORT ${SVTKm_EXPORT_NAME})

  ## Install headers
if(NOT SVTKm_INSTALL_ONLY_LIBRARIES)
  install(DIRECTORY svtkmdiy
    DESTINATION ${SVTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
  install(FILES
    ${SVTKm_BINARY_INCLUDE_DIR}/${kit_dir}/Configure.h
    ${CMAKE_CURRENT_SOURCE_DIR}/diy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/serialization.h
    DESTINATION ${SVTKm_INSTALL_INCLUDE_DIR}/${kit_dir}/)
endif()
