cmake_minimum_required(VERSION 3.8)
project(UsingSVTK)

# We just need the CommonCore module here.
find_package(SVTK COMPONENTS CommonCore)
if (NOT SVTK_FOUND)
  message("Skipping example: ${SVTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

include(CTest)

add_library(svtk_using_lib uses_svtk.cxx)
# SVTK modules act just like CMake imported targets.
target_link_libraries(svtk_using_lib PRIVATE SVTK::CommonCore)
target_include_directories(svtk_using_lib PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
include(GenerateExportHeader)
generate_export_header(svtk_using_lib)

add_executable(svtk_using_exe uses_svtk_exe.cxx)
target_link_libraries(svtk_using_exe PRIVATE svtk_using_lib)

add_test(NAME svtk_using_exe
  COMMAND svtk_using_exe)
