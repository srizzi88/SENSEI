svtk_add_test_cxx(svtkViewsInfovisCxxTests tests
  TestConeLayoutStrategy.cxx
  TestCoincidentGraphLayoutView.cxx
  TestColumnTree.cxx
  TestDendrogramItem.cxx
  TestGraphLayoutView.cxx
  TestHeatmapCategoryLegend.cxx
  TestHeatmapItem.cxx
  TestHeatmapScalarLegend.cxx
  TestHierarchicalGraphView.cxx
  TestIcicleView.cxx
  TestIconGlyphFilter.cxx
  TestInteractorStyleTreeMapHover.cxx
  TestNetworkViews.cxx
  TestParallelCoordinatesView.cxx
  TestRenderView.cxx
  TestSpanTreeLayoutStrategy.cxx
  TestTanglegramItem.cxx
  TestTreeHeatmapAutoCollapse.cxx
  TestTreeHeatmapItem.cxx
  TestTreeMapView.cxx
  TestTreeRingView.cxx
  )

svtk_test_cxx_executable(svtkViewsInfovisCxxTests tests
  RENDERING_FACTORY)
