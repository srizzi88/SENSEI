cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(MultiBlock)

find_package(SVTK
  COMPONENTS
    FiltersGeneral
    FiltersGeometry
    FiltersSources
    IOXML
    RenderingCore
    TestingCore)
if (NOT SVTK_FOUND)
  message("Skipping example: ${SVTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(MultiBlock MACOSX_BUNDLE MultiBlock.cxx)
target_link_libraries(MultiBlock
  PRIVATE
    ${SVTK_LIBRARIES})
svtk_module_autoinit(
  TARGETS MultiBlock
  MODULES ${SVTK_LIBRARIES})
