##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================

# If you want CUDA support, you will need to have CMake 3.9 on Linux/OSX.
# We require CMake 3.11 with the MSVC generator as the $<COMPILE_LANGUAGE:>
# generator expression is not supported on older versions.
cmake_minimum_required(VERSION 3.8...3.15 FATAL_ERROR)
project (SVTKm)

if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
  cmake_minimum_required(VERSION 3.11...3.15 FATAL_ERROR)
endif()

# Update module path
set(SVTKm_CMAKE_MODULE_PATH ${SVTKm_SOURCE_DIR}/CMake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${SVTKm_CMAKE_MODULE_PATH})

# Determine SVTK-m version
include(Utilities/Git/Git.cmake)
include(SVTKmDetermineVersion)

# Load hardcoded version in case this is not a Git repository
file(STRINGS version.txt version_txt)
extract_version_components("${version_txt}" "SVTKm")
# Get the version from git if we can
determine_version(${SVTKm_SOURCE_DIR} ${GIT_EXECUTABLE} "SVTKm")

if (NOT DEFINED SVTKm_INSTALL_INCLUDE_DIR)
  set(SVTKm_INSTALL_INCLUDE_DIR "include/svtkm-${SVTKm_VERSION_MAJOR}.${SVTKm_VERSION_MINOR}")
endif()
if (NOT DEFINED SVTKm_INSTALL_CONFIG_DIR)
  set(SVTKm_INSTALL_CONFIG_DIR "lib/cmake/svtkm-${SVTKm_VERSION_MAJOR}.${SVTKm_VERSION_MINOR}")
endif()
if (NOT DEFINED SVTKm_INSTALL_LIB_DIR)
  set(SVTKm_INSTALL_LIB_DIR "lib")
endif()
if (NOT DEFINED SVTKm_INSTALL_BIN_DIR)
  set(SVTKm_INSTALL_BIN_DIR "bin")
endif()
if (NOT DEFINED SVTKm_INSTALL_SHARE_DIR)
  set(SVTKm_INSTALL_SHARE_DIR "share/svtkm-${SVTKm_VERSION_MAJOR}.${SVTKm_VERSION_MINOR}")
endif()
if (NOT DEFINED SVTKm_INSTALL_CMAKE_MODULE_DIR)
  set(SVTKm_INSTALL_CMAKE_MODULE_DIR "${SVTKm_INSTALL_SHARE_DIR}/cmake")
endif()
if (NOT DEFINED SVTKm_BUILD_CMAKE_BASE_DIR)
  set(SVTKm_BUILD_CMAKE_BASE_DIR "${SVTKm_BINARY_DIR}")
endif()
if(NOT DEFINED SVTKm_EXECUTABLE_OUTPUT_PATH)
  ## Set the directory where the binaries will be stored
  set(SVTKm_EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
endif()
if(NOT DEFINED SVTKm_LIBRARY_OUTPUT_PATH)
  ## Set the directory where the libraries will be stored
  set(SVTKm_LIBRARY_OUTPUT_PATH  ${PROJECT_BINARY_DIR}/lib)
endif()
if (NOT DEFINED SVTKm_EXPORT_NAME)
  set(SVTKm_EXPORT_NAME "SVTKmTargets")
endif()

set(SVTKm_BINARY_INCLUDE_DIR "${SVTKm_BINARY_DIR}/include")

#-----------------------------------------------------------------------------
# svtkm_option(variable doc [initial])
#   Provides an option if it is not already defined.
# This can be replaced when CMake 3.13 is our cmake_minimum_required
macro (svtkm_option variable)
  if (NOT DEFINED "${variable}")
    option("${variable}" ${ARGN})
  endif ()
endmacro ()

# Configurable Options
svtkm_option(SVTKm_ENABLE_CUDA "Enable Cuda support" OFF)
svtkm_option(SVTKm_ENABLE_TBB "Enable TBB support" OFF)
svtkm_option(SVTKm_ENABLE_OPENMP "Enable OpenMP support" OFF)
svtkm_option(SVTKm_ENABLE_RENDERING "Enable rendering library" ON)
svtkm_option(SVTKm_ENABLE_BENCHMARKS "Enable SVTKm Benchmarking" OFF)
svtkm_option(SVTKm_ENABLE_MPI "Enable MPI support" OFF)
svtkm_option(SVTKm_ENABLE_DOCUMENTATION "Build Doxygen documentation" OFF)
svtkm_option(SVTKm_ENABLE_EXAMPLES "Build examples" OFF)
if (NOT DEFINED SVTKm_ENABLE_TESTING)
    if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
      svtkm_option(SVTKm_ENABLE_TESTING "Enable SVTKm Testing" ON)
    else()
      svtkm_option(SVTKm_ENABLE_TESTING "Enable SVTKm Testing" OFF)
    endif()
endif()

svtkm_option(SVTKm_USE_DOUBLE_PRECISION "Use double precision for floating point calculations" OFF)
svtkm_option(SVTKm_USE_64BIT_IDS "Use 64-bit indices." ON)

# SVTK-m will turn on logging by default, but will set the default
# logging level to WARN.  This option should not be visible by default
# in the GUI, as ERROR and WARN level logging should not interfere
# with the performance of svtk-m
svtkm_option(SVTKm_ENABLE_LOGGING "Enable SVTKm Logging" ON)

# When SVTK-m is embedded into larger projects they may desire to turn off
# SVTK-m internal assert checks when in debug mode to improve debug runtime
# performance.
svtkm_option(SVTKm_NO_ASSERT "Disable assertions in debugging builds." OFF)

# When SVTK-m is embedded into larger projects that wish to make end user
# applications they want to only install libraries and don't want CMake/headers
# installed.
svtkm_option(SVTKm_INSTALL_ONLY_LIBRARIES "install only svtk-m libraries and no headers" OFF)

# SVTK-m is setup by default not to export symbols unless explicitly stated.
# We prefer to only export symbols of a small set of user facing classes,
# rather than exporting all symbols. This flag is added so that consumers
# which require static builds can force all symbols on, which is something
# SVTK does.
svtkm_option(SVTKm_USE_DEFAULT_SYMBOL_VISIBILITY "Don't explicitly hide symbols from libraries." OFF)

svtkm_option(BUILD_SHARED_LIBS "Build SVTK-m with shared libraries" OFF)
set(SVTKm_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})

# This flag can be used to prevent SVTK-m from exporting its warning flags in its
# build interface. This is useful when building SVTK-m as a thirdparty library
# and the warnings are too strict for the parent project.
svtkm_option(SVTKm_ENABLE_DEVELOPER_FLAGS "Enable compiler flags that are useful while developing SVTK-m" ON)

mark_as_advanced(
  SVTKm_ENABLE_LOGGING
  SVTKm_NO_ASSERT
  SVTKm_INSTALL_ONLY_LIBRARIES
  SVTKm_USE_DEFAULT_SYMBOL_VISIBILITY
  SVTKm_ENABLE_DEVELOPER_FLAGS
  )

#-----------------------------------------------------------------------------
# When using C++11 support make sure you use the standard C++ extensions rather
# than compiler-specific versions of the extensions (to preserve portability).
set(CMAKE_CXX_EXTENSIONS Off)

# Setup default build types
include(SVTKmBuildType)

# Include the svtk-m wrappers
include(SVTKmWrappers)

# Create svtkm_compiler_flags library. This is an interface library that
# holds all the C++ compiler flags that are needed for consumers and
# when building SVTK-m.
include(SVTKmCompilerFlags)


#-----------------------------------------------------------------------------
# We include the wrappers unconditionally as SVTK-m expects the function to
# always exist (and early terminate when testing is disabled).
include(testing/SVTKmTestWrappers)
if (SVTKm_ENABLE_TESTING)
  enable_testing()
  # Only include CTest if it has not been included by a superproject. The
  # variable DEFAULT_CTEST_CONFIGURATION_TYPE is a non-cached variable set by
  # CTest.cmake, so we'll use that to determine if it's already included.
  if(NOT DEFINED DEFAULT_CTEST_CONFIGURATION_TYPE)
    include(CTest)
    # Mark this as advanced to avoid confusion, since we actually rely on
    # SVTKm_ENABLE_TESTING.
    mark_as_advanced(BUILD_TESTING)
  endif()


  configure_file(${SVTKm_SOURCE_DIR}/CTestCustom.cmake.in
    ${SVTKm_BINARY_DIR}/CTestCustom.cmake @ONLY)

  #-----------------------------------------------------------------------------
  # Find the Python interpreter, which we will use during the build process
  find_package(PythonInterp QUIET)

  #-----------------------------------------------------------------------------
  # Find Pyexpander in case somebody wants to update the auto generated
  # faux variadic template code
  find_package(Pyexpander QUIET)

  #-----------------------------------------------------------------------------
  # Setup compiler flags for dynamic analysis if needed
  include(SVTKmCompilerDynamicAnalysisFlags)
  svtkm_check_sanitizer_build()
endif (SVTKm_ENABLE_TESTING)

#-----------------------------------------------------------------------------
# Check basic type sizes.
include(CheckTypeSize)

check_type_size(long SVTKm_SIZE_LONG BUILTIN_TYPES_ONLY)
check_type_size("long long" SVTKm_SIZE_LONG_LONG BUILTIN_TYPES_ONLY)

#-----------------------------------------------------------------------------
# Add subdirectories
add_subdirectory(svtkm)

#-----------------------------------------------------------------------------
# Build documentation
if (SVTKm_ENABLE_DOCUMENTATION)
  include(SVTKmBuildDocumentation)
endif()

#-----------------------------------------------------------------------------
# Ready files for find_package
include(CMakePackageConfigHelpers)

configure_package_config_file(
  ${SVTKm_SOURCE_DIR}/CMake/SVTKmConfig.cmake.in
  ${SVTKm_BUILD_CMAKE_BASE_DIR}/${SVTKm_INSTALL_CONFIG_DIR}/SVTKmConfig.cmake
  INSTALL_DESTINATION ${SVTKm_INSTALL_CONFIG_DIR}
  PATH_VARS
    SVTKm_INSTALL_INCLUDE_DIR
    SVTKm_INSTALL_CONFIG_DIR
    SVTKm_INSTALL_LIB_DIR
    SVTKm_INSTALL_BIN_DIR
    SVTKm_INSTALL_CMAKE_MODULE_DIR
  )

write_basic_package_version_file(
  ${SVTKm_BUILD_CMAKE_BASE_DIR}/${SVTKm_INSTALL_CONFIG_DIR}/SVTKmConfigVersion.cmake
  VERSION ${SVTKm_VERSION}
  COMPATIBILITY ExactVersion )

if(NOT SVTKm_INSTALL_ONLY_LIBRARIES)
  install(
    FILES
      ${SVTKm_BUILD_CMAKE_BASE_DIR}/${SVTKm_INSTALL_CONFIG_DIR}/SVTKmConfig.cmake
      ${SVTKm_BUILD_CMAKE_BASE_DIR}/${SVTKm_INSTALL_CONFIG_DIR}/SVTKmConfigVersion.cmake
    DESTINATION ${SVTKm_INSTALL_CONFIG_DIR}
    )

  # Install the readme and license files.
  install(FILES ${SVTKm_SOURCE_DIR}/README.md
    DESTINATION ${SVTKm_INSTALL_SHARE_DIR}
    RENAME SVTKmREADME.md
    )
  install(FILES ${SVTKm_SOURCE_DIR}/LICENSE.txt
    DESTINATION ${SVTKm_INSTALL_SHARE_DIR}
    RENAME SVTKmLICENSE.txt
    )

  # Install helper configure files.
  install(
    FILES
      ${SVTKm_SOURCE_DIR}/CMake/FindTBB.cmake
      ${SVTKm_SOURCE_DIR}/CMake/FindMPI.cmake
      ${SVTKm_SOURCE_DIR}/CMake/FindOpenGL.cmake
    DESTINATION ${SVTKm_INSTALL_CMAKE_MODULE_DIR}
    )

  # Install support files.
  install(
    FILES
      ${SVTKm_SOURCE_DIR}/CMake/SVTKmCPUVectorization.cmake
      ${SVTKm_SOURCE_DIR}/CMake/SVTKmDetectCUDAVersion.cu
      ${SVTKm_SOURCE_DIR}/CMake/SVTKmDeviceAdapters.cmake
      ${SVTKm_SOURCE_DIR}/CMake/SVTKmExportHeaderTemplate.h.in
      ${SVTKm_SOURCE_DIR}/CMake/SVTKmMPI.cmake
      ${SVTKm_SOURCE_DIR}/CMake/SVTKmRenderingContexts.cmake
      ${SVTKm_SOURCE_DIR}/CMake/SVTKmWrappers.cmake
    DESTINATION ${SVTKm_INSTALL_CMAKE_MODULE_DIR}
    )

  # Create and install exports for external projects
  export(EXPORT ${SVTKm_EXPORT_NAME}
    FILE ${SVTKm_BUILD_CMAKE_BASE_DIR}/${SVTKm_INSTALL_CONFIG_DIR}/SVTKmTargets.cmake
    )
  install(EXPORT ${SVTKm_EXPORT_NAME}
    DESTINATION ${SVTKm_INSTALL_CONFIG_DIR}
    FILE SVTKmTargets.cmake
    )
endif()

svtkm_option(SVTKm_ENABLE_CPACK "Enable CPack packaging of SVTKm" ON)
if (SVTKm_ENABLE_CPACK)
  # Enable CPack packaging
  set(CPACK_PACKAGE_DESCRIPTION_FILE ${SVTKm_SOURCE_DIR}/README.md)
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The SVTKm Toolkit")
  set(CPACK_PACKAGE_NAME "SVTKm")
  set(CPACK_PACKAGE_VERSION_MAJOR ${SVTKm_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${SVTKm_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${SVTKm_VERSION_PATCH})
  set(CPACK_PACKAGE_FILE_NAME "SVTKm-${SVTKm_VERSION}")
  set(CPACK_RESOURCE_FILE_LICENSE ${SVTKm_SOURCE_DIR}/LICENSE.txt)
  set(CPACK_RESOURCE_FILE_README ${SVTKm_SOURCE_DIR}/README.md)
  include(CPack)
endif ()

#-----------------------------------------------------------------------------
#add the benchmarking folder
if(SVTKm_ENABLE_BENCHMARKS)
    add_subdirectory(benchmarking)
endif()

#-----------------------------------------------------------------------------
if (SVTKm_ENABLE_TESTING)

  #-----------------------------------------------------------------------------
  # Add "meta" tests that check the state of the repository
  # SystemInformation prints out information about the current configuration
  # CopyrightStatement checks that the copyright statement is in all source files
  # SourceInBuild checks that all source files are listed in the build
  # SourceInInstall checks that all source files are installed in the build
  add_test(NAME SystemInformation
    COMMAND ${CMAKE_COMMAND} "-DSVTKm_SOURCE_DIR=${SVTKm_SOURCE_DIR}" "-DSVTKm_BINARY_DIR=${SVTKm_BINARY_DIR}" -P "${SVTKm_SOURCE_DIR}/CMake/testing/SVTKmSystemInformation.cmake"
    )
  add_test(NAME CopyrightStatement
    COMMAND ${CMAKE_COMMAND} "-DSVTKm_SOURCE_DIR=${SVTKm_SOURCE_DIR}" -P "${SVTKm_SOURCE_DIR}/CMake/SVTKmCheckCopyright.cmake"
    )
  # increase timeout since on some machines CopyrightStatement test takes a long time.
  set_tests_properties(CopyrightStatement PROPERTIES TIMEOUT 300)

  # Setup the infrastructure to allow SVTK-m to run tests against a temporary
  # installed version of SVTK-m.
  include(testing/SVTKmTestInstall)
  svtkm_test_install()
endif()

#-----------------------------------------------------------------------------
# Build examples
add_subdirectory(examples)
