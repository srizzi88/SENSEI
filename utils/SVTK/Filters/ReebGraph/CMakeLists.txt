set(classes
  svtkReebGraphSimplificationFilter
  svtkReebGraphSurfaceSkeletonFilter
  svtkReebGraphToJoinSplitTreeFilter
  svtkReebGraphVolumeSkeletonFilter
  svtkUnstructuredGridToReebGraphFilter)

svtk_module_add_module(SVTK::FiltersReebGraph
  CLASSES ${classes})

svtk_module_find_package(PRIVATE
  PACKAGE Boost)
svtk_module_link(SVTK::FiltersReebGraph
  PRIVATE
    Boost::boost)
