cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(DataManipulation)

find_package(SVTK
  COMPONENTS
    CommonColor
    FiltersGeometry
    RenderingCore)
if (NOT SVTK_FOUND)
  message("Skipping example: ${SVTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(Arrays MACOSX_BUNDLE Arrays.cxx)
add_executable(Cube MACOSX_BUNDLE Cube.cxx)
add_executable(SGrid MACOSX_BUNDLE SGrid.cxx)
add_executable(RGrid MACOSX_BUNDLE RGrid.cxx)

target_link_libraries(Arrays PRIVATE ${SVTK_LIBRARIES})
target_link_libraries(Cube PRIVATE ${SVTK_LIBRARIES})
target_link_libraries(SGrid PRIVATE ${SVTK_LIBRARIES})
target_link_libraries(RGrid PRIVATE ${SVTK_LIBRARIES})

svtk_module_autoinit(
  TARGETS Arrays
          Cube
          SGrid
          RGrid
  MODULES ${SVTK_LIBRARIES})
