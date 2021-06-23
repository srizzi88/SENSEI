cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


PROJECT (Step5)

find_package(SVTK REQUIRED)
svtk_module_config(SVTK
  svtkCommonCore
  svtkFiltersSources
  svtkInteractionStyle
  svtkRenderingOpenGL2
)
include(${SVTK_USE_FILE})

add_executable(Cone5 MACOSX_BUNDLE Cone5.cxx)
target_link_libraries(Cone5 ${SVTK_LIBRARIES})
