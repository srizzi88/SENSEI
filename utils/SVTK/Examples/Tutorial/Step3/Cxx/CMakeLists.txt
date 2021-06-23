cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


project (Step3)

find_package(SVTK REQUIRED)
svtk_module_config(SVTK
  svtkCommonCore
  svtkFiltersSources
  svtkInteractionStyle
  svtkRenderingOpenGL2
)
include(${SVTK_USE_FILE})

add_executable(Cone3 MACOSX_BUNDLE Cone3.cxx)
target_link_libraries(Cone3 ${SVTK_LIBRARIES})
