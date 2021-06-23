svtk_add_test_cxx(svtkFiltersAMRCxxTests tests
  TestAMRSliceFilterCellData.cxx
  TestAMRSliceFilterPointData.cxx
  TestAMRGhostLayerStripping.cxx,NO_VALID
  TestAMRBlanking.cxx,NO_VALID
  TestAMRIterator.cxx,NO_VALID
  TestImageToAMR.cxx,NO_VALID
  )

svtk_test_cxx_executable(svtkFiltersAMRCxxTests tests)
