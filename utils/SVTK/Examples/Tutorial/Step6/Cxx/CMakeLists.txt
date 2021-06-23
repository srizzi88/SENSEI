cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


PROJECT (Step6)

find_package(SVTK REQUIRED)
svtk_module_config(SVTK
  svtkCommonCore
  svtkFiltersSources
  svtkInteractionStyle
  svtkInteractionWidgets
  svtkRenderingOpenGL2
)
include(${SVTK_USE_FILE})

add_executable(Cone6 MACOSX_BUNDLE Cone6.cxx)
target_link_libraries(Cone6 ${SVTK_LIBRARIES})
