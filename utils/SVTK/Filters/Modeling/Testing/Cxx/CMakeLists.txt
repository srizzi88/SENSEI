svtk_add_test_cxx(svtkFiltersModelingCxxTests tests
  TestButterflyScalars.cxx
  TestDijkstraGraphGeodesicPath.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestLinearCellExtrusion.cxx
  TestNamedColorsIntegration.cxx
  TestPolyDataPointSampler.cxx
  TestQuadRotationalExtrusion.cxx
  TestQuadRotationalExtrusionMultiBlock.cxx
  TestRotationalExtrusion.cxx
  TestSelectEnclosedPoints.cxx
  TestVolumeOfRevolutionFilter.cxx
  UnitTestCollisionDetectionFilter.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  UnitTestHausdorffDistancePointSetFilter.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  UnitTestSubdivisionFilters.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  )
svtk_add_test_cxx(svtkFiltersModelingCxxTests tests
  TestCollisionDetectionAllContacts,TestCollisionDetection.cxx 0)
svtk_add_test_cxx(svtkFiltersModelingCxxTests tests
  TestCollisionDetectionFirstContact,TestCollisionDetection.cxx 1)
svtk_add_test_cxx(svtkFiltersModelingCxxTests tests
  TestCollisionDetectionHalfContacts,TestCollisionDetection.cxx 2)

svtk_test_cxx_executable(svtkFiltersModelingCxxTests tests RENDERING_FACTORY)
