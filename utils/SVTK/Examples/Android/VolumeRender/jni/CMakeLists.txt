set(sources
  main.cxx
)

add_library(VolumeRender SHARED ${sources})
target_link_libraries(VolumeRender ${SVTK_LIBRARIES}
  android
  log
)
