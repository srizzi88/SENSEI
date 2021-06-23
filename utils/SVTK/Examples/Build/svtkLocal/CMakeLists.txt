cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)

project(svtkLocalExample)

# Find and load SVTK settings.
if(NOT SVTK_BINARY_DIR)
  find_package(SVTK REQUIRED svtkCommonCore)
  include(${SVTK_USE_FILE})
endif()

include(${SVTK_CMAKE_DIR}/svtkExternalModuleMacros.cmake)

svtk_module(svtkLocalExample
  DEPENDS
  svtkCommonCore
)

include_directories(SYSTEM
  ${SVTK_INCLUDE_DIRS}
)

# Source files.
set(svtkLOCAL_SRCS
  svtkLocalExample.cxx
)

# Build and link library.
svtk_module_library(svtkLocalExample ${svtkLOCAL_SRCS})
