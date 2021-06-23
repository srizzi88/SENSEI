cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


project (Step2)
find_package(SVTK REQUIRED)
svtk_module_config(SVTK
  svtkCommonCore
  svtkFiltersSources
  svtkFiltersSources
  svtkInteractionStyle
  svtkRenderingOpenGL2
)
include(${SVTK_USE_FILE})

add_executable(Cone2 MACOSX_BUNDLE Cone2.cxx)
target_link_libraries(Cone2 ${SVTK_LIBRARIES})
