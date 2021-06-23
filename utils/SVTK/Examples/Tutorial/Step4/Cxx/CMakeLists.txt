cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


project (Step4)

find_package(SVTK REQUIRED)
svtk_module_config(SVTK
  svtkCommonCore
  svtkFiltersSources
  svtkInteractionStyle
  svtkRenderingOpenGL2
)
include(${SVTK_USE_FILE})

add_executable(Cone4 MACOSX_BUNDLE Cone4.cxx)
target_link_libraries(Cone4 ${SVTK_LIBRARIES})
