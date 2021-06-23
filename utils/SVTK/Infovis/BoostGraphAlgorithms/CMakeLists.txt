set(classes
  svtkBoostBetweennessClustering
  svtkBoostBiconnectedComponents
  svtkBoostBrandesCentrality
  svtkBoostBreadthFirstSearch
  svtkBoostBreadthFirstSearchTree
  svtkBoostConnectedComponents
  svtkBoostDividedEdgeBundling
  svtkBoostExtractLargestComponent
  svtkBoostKruskalMinimumSpanningTree
  svtkBoostLogWeighting
  svtkBoostPrimMinimumSpanningTree
  svtkBoostRandomSparseArraySource
  svtkBoostSplitTableField)

set(headers
  svtkBoostGraphAdapter.h)

svtk_module_add_module(SVTK::InfovisBoostGraphAlgorithms
  CLASSES ${classes}
  HEADERS ${headers})

svtk_module_find_package(
  PACKAGE Boost
  FORWARD_VERSION_REQ EXACT
  VERSION_VAR         "@Boost_MAJOR_VERSION@.@Boost_MINOR_VERSION@.@Boost_SUBMINOR_VERSION@")
svtk_module_link(SVTK::InfovisBoostGraphAlgorithms
  PUBLIC
    Boost::boost)
