cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


project (Step1)

find_package(SVTK REQUIRED)
svtk_module_config(SVTK
  svtkCommonCore
  svtkFiltersSources
  svtkFiltersSources
  svtkInteractionStyle
  svtkRenderingOpenGL2
)
include(${SVTK_USE_FILE})
add_executable(Cone MACOSX_BUNDLE Cone.cxx)
target_link_libraries(Cone ${SVTK_LIBRARIES})
