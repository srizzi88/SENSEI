cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


project (VolumeRendering)
find_package(SVTK REQUIRED)
svtk_module_config(SVTK
  svtkCommonCore
  svtkIOImage
  svtkIOXML
  svtkInteractionWidgets
  svtkRenderingVolumeOpenGL2
)
include(${SVTK_USE_FILE})

set(RENDERING_EXAMPLES_SRCS
  GPURenderDemo
  FixedPointVolumeRayCastMapperCT
)

foreach(name ${RENDERING_EXAMPLES_SRCS})
  add_executable(${name} MACOSX_BUNDLE ${name}.cxx)
  target_link_libraries(${name} ${SVTK_LIBRARIES})
endforeach()



