cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


PROJECT (Rendering)

if(NOT SVTK_SOURCE_DIR)
  message(ERROR " Cannot build Rendering examples without SVTK_SOURCE_DIR")
endif()
find_package(SVTK REQUIRED)
svtk_module_config(SVTK
  svtkCommonCore
  svtkFiltersSources
  svtkInteractionStyle
  svtkRenderingOpenGL2
)
include(${SVTK_USE_FILE})

set(RENDERING_EXAMPLES_SRCS
  Cylinder
  SpecularSpheres
  DiffuseSpheres
  AmbientSpheres
  )

foreach(name ${RENDERING_EXAMPLES_SRCS})
  add_executable(${name} MACOSX_BUNDLE ${name}.cxx)
  target_link_libraries(${name} ${SVTK_LIBRARIES} )
endforeach()
