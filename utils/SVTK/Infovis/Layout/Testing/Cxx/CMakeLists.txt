svtk_add_test_cxx(svtkInfovisLayoutCxxTests tests
  # TestAssignCoordinates.cxx # todo (unsatisfied deps)
  TestChacoGraphReader.cxx
  TestCirclePackLayoutStrategy.cxx
  # TestCosmicTreeLayoutStrategy.cxx # todo (unsatisfied deps)
  TestGraphLayoutStrategy.cxx,NO_VALID
  TestIncrementalForceLayout.cxx,NO_VALID
  TestTreeMapLayoutStrategy.cxx
  )
svtk_test_cxx_executable(svtkInfovisLayoutCxxTests tests)
