svtk_add_test_cxx(svtkFiltersHybridCxxTests tests
  TemporalStatistics.cxx
  TestHyperTreeGridBinary2DAdaptiveDataSetSurfaceFilter.cxx
  TestHyperTreeGridBinary2DAdaptiveDataSetSurfaceFilterMaterial.cxx
  TestHyperTreeGridTernary3DAdaptiveDataSetSurfaceFilter.cxx
  TestHyperTreeGridTernary3DToUnstructuredAdaptiveDataSetSurfaceFilter.cxx
  TestHyperTreeGridTernary3DAdaptiveDataSetSurfaceFilterMaterial.cxx
  TestBSplineTransform.cxx
  TestDepthSortPolyData.cxx
  TestForceTime.cxx
  TestPolyDataSilhouette.cxx
  TestProcrustesAlignmentFilter.cxx,NO_VALID
  TestTemporalArrayOperatorFilter.cxx,NO_VALID
  TestTemporalCacheSimple.cxx,NO_VALID
  TestTemporalCacheTemporal.cxx,NO_VALID
  TestTemporalFractal.cxx
  )
svtk_test_cxx_executable(svtkFiltersHybridCxxTests tests
  RENDERING_FACTORY
  )
