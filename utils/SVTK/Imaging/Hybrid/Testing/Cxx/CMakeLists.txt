svtk_add_test_cxx(svtkImagingHybridCxxTests tests
  TestImageToPoints.cxx
  TestSampleFunction.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )
svtk_test_cxx_executable(svtkImagingHybridCxxTests tests
  RENDERING_FACTORY
  )
