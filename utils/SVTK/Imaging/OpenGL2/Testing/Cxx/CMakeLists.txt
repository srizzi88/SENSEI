svtk_add_test_cxx(svtkImagingOpenGL2CxxTests tests
  TestOpenGLImageGradient.cxx
  )

svtk_test_cxx_executable(svtkImagingOpenGL2CxxTests tests RENDERING_FACTORY)
