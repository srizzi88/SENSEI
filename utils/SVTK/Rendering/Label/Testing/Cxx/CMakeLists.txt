svtk_add_test_cxx(svtkRenderingLabelCxxTests tests
  TestClipLabels.cxx
  TestDynamic2DLabelMapper.cxx
  TestLabelPlacer.cxx
  TestLabelPlacer2D.cxx
  TestLabelPlacerCoincidentPoints.cxx
  TestLabelPlacementMapper.cxx
  TestLabelPlacementMapper2D.cxx
  TestLabelPlacementMapperCoincidentPoints.cxx
  )
svtk_test_cxx_executable(svtkRenderingLabelCxxTests tests)
