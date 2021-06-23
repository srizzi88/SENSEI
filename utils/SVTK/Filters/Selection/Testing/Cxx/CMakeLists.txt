svtk_add_test_cxx(svtkFiltersSelectionCxxTests tests
  NO_VALID
  TestLinearSelector2D.cxx
  TestCellDistanceSelector2D.cxx
  TestCellDistanceSelector3D.cxx
  TestLinearSelector3D.cxx
  )
svtk_test_cxx_executable(svtkFiltersSelectionCxxTests tests
  RENDERING_FACTORY
  )
