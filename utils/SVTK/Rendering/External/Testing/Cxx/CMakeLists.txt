if(NOT APPLE)
  # GLUT tests don't work on Apple.
  # see discussion on svtk/svtk!5169

  # TODO: Make FindGLUT use imported targets.
  find_package(GLUT REQUIRED)
  include_directories(${GLUT_INCLUDE_DIR})

  svtk_add_test_cxx(svtkRenderingExternalCxxTests tests
    TestGLUTRenderWindow.cxx
    )

  svtk_test_cxx_executable(svtkRenderingExternalCxxTests tests)
  target_link_libraries(svtkRenderingExternalCxxTests PRIVATE
    ${GLUT_LIBRARY})
endif()
