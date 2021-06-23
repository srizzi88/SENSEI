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
project(PolyLineArchimedeanHelix CXX)

find_package(SVTKm REQUIRED QUIET)
if (SVTKm_ENABLE_RENDERING)
    add_executable(PolyLineArchimedeanHelix PolyLineArchimedeanHelix.cxx)
    svtkm_add_target_information(PolyLineArchimedeanHelix
                                DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                                DEVICE_SOURCES PolyLineArchimedeanHelix.cxx)
    target_link_libraries(PolyLineArchimedeanHelix PRIVATE svtkm_filter svtkm_rendering)
endif()
