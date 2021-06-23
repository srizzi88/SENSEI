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
project(RedistributePoints CXX)

#Find the SVTK-m package
find_package(SVTKm REQUIRED QUIET)
add_executable(RedistributePoints RedistributePoints.cxx RedistributePoints.h)
target_link_libraries(RedistributePoints PRIVATE svtkm_filter)
svtkm_add_target_information(RedistributePoints
                            DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES RedistributePoints.cxx)

set_property(TARGET RedistributePoints PROPERTY CUDA_VISIBILITY_PRESET "hidden")
set_property(TARGET RedistributePoints PROPERTY CXX_VISIBILITY_PRESET "hidden")
