cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(LabeledMesh)

find_package(SVTK
  COMPONENTS
    FiltersGeneral
    FiltersSources
    RenderingCore
    RenderingLabel)
if (NOT SVTK_FOUND)
  message("Skipping example: ${SVTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

add_executable(LabeledMesh MACOSX_BUNDLE
  LabeledMesh.cxx)
target_link_libraries(LabeledMesh
  PRIVATE
    ${SVTK_LIBRARIES})
svtk_module_autoinit(
  TARGETS LabeledMesh
  MODULES ${SVTK_LIBRARIES})
