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
project(ParticleAdvection CXX)

#Find the SVTK-m package
find_package(SVTKm REQUIRED QUIET)

add_executable(Particle_Advection ParticleAdvection.cxx)
target_link_libraries(Particle_Advection PRIVATE svtkm_filter)
svtkm_add_target_information(Particle_Advection
                            DROP_UNUSED_SYMBOLS MODIFY_CUDA_FLAGS
                            DEVICE_SOURCES ParticleAdvection.cxx)
if(TARGET svtkm::tbb)
  target_compile_definitions(Particle_Advection PRIVATE BUILDING_TBB_VERSION)
endif()
