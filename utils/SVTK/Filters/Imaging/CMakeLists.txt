set(classes
  svtkComputeHistogram2DOutliers
  svtkExtractHistogram2D
  svtkPairwiseExtractHistogram2D)

svtk_module_add_module(SVTK::FiltersImaging
  CLASSES ${classes})
