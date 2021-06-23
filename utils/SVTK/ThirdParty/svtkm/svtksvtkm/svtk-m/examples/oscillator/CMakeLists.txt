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
project(Oscillator CXX)

#Find the SVTK-m package
find_package(SVTKm REQUIRED QUIET)

add_executable(Oscillator Oscillator.cxx)
target_link_libraries(Oscillator PRIVATE svtkm_source)
svtkm_add_target_information(Oscillator
                            DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES Oscillator.cxx)
