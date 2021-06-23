svtk_add_test_cxx(svtkCommonColorCxxTests tests
  TestCategoricalColors.cxx,NO_VALID
  TestColorSeries.cxx
  TestColorSeriesLookupTables.cxx,NO_VALID
  TestNamedColors.cxx,NO_VALID
  )
svtk_test_cxx_executable(svtkCommonColorCxxTests tests)
