set(classes
  svtkHyperTreeGridAxisClip
  svtkHyperTreeGridAxisCut
  svtkHyperTreeGridAxisReflection
  svtkHyperTreeGridCellCenters
  svtkHyperTreeGridContour
  svtkHyperTreeGridDepthLimiter
  svtkHyperTreeGridEvaluateCoarse
  svtkHyperTreeGridGeometry
  svtkHyperTreeGridPlaneCutter
  svtkHyperTreeGridThreshold
  svtkHyperTreeGridToDualGrid
  svtkHyperTreeGridToUnstructuredGrid
  svtkImageDataToHyperTreeGrid
)

svtk_module_add_module(SVTK::FiltersHyperTree
  CLASSES ${classes})
