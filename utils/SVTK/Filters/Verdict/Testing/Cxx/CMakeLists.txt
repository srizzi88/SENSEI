svtk_add_test_cxx(svtkFiltersVerdictCxxTests tests
  NO_VALID
  CellSizeFilter.cxx
  CellSizeFilter2.cxx
  MeshQuality.cxx
  )
svtk_test_cxx_executable(svtkFiltersVerdictCxxTests tests)
