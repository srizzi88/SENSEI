include_directories(
  "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue"
)

set(sources
  main.cxx
  ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
)

add_library(NativeSVTK SHARED ${sources})
target_link_libraries(NativeSVTK ${SVTK_LIBRARIES}
  android
  log
)
