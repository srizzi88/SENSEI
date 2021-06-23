svtk_add_test_cxx(svtkImagingMorphologicalCxxTests tests
  TestImageThresholdConnectivity.cxx
  TestImageConnectivityFilter.cxx
  )

svtk_test_cxx_executable(svtkImagingMorphologicalCxxTests tests
  RENDERING_FACTORY
  )
