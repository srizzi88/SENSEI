svtk_add_test_cxx(svtkFiltersPointsCxxTests tests
  UnitTestKernels.cxx,NO_VALID
  TestSPHKernels.cxx,NO_VALID
  PlotSPHKernels.cxx
  TestPointCloudFilterArrays.cxx,NO_VALID,NO_DATA
  )
svtk_test_cxx_executable(svtkFiltersPointsCxxTests tests
  RENDERING_FACTORY
  )
