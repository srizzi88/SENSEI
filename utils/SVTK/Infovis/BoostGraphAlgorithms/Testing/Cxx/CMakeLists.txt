svtk_add_test_cxx(svtkInfovisBoostGraphAlgorithmsCxxTests tests
  # BoostArrayLogWeighting.cxx    # todo (unsatisfied deps)
  BoostArrayRandomSparseArraySource.cxx,NO_VALID
  TestBoostAdapter.cxx,NO_VALID
  TestBoostAlgorithms.cxx
  TestBoostBetweennessClustering.cxx,NO_VALID
  # TestBoostBrandesCentrality.cxx # todo (unsatisfied deps)
  TestBoostDividedEdgeBundling.cxx
  TestBoostExtractLargestComponent.cxx,NO_VALID
  TestBoostSplitTableField.cxx,NO_VALID
  )
svtk_test_cxx_executable(svtkInfovisBoostGraphAlgorithmsCxxTests tests)

find_package(Boost REQUIRED)
target_link_libraries(svtkInfovisBoostGraphAlgorithmsCxxTests
  PRIVATE
    Boost::boost)
