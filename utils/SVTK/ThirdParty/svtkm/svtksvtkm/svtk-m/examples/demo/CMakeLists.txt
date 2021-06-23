##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
cmake_minimum_required(VERSION 3.8...3.15 FATAL_ERROR)
project(SVTKmDemo CXX)

#Find the SVTK-m package
find_package(SVTKm REQUIRED QUIET)

if(TARGET svtkm_rendering)
  add_executable(Demo Demo.cxx)
  target_link_libraries(Demo PRIVATE svtkm_filter svtkm_rendering)
  svtkm_add_target_information(Demo
                              DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                              DEVICE_SOURCES Demo.cxx)
endif()
