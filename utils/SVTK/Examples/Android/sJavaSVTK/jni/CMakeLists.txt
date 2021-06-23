set(sources
  main.cxx
)

add_library(JavaSVTK SHARED ${sources})
target_link_libraries(JavaSVTK ${SVTK_LIBRARIES}
  android
  log
)
