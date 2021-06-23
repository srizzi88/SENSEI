set(classes
  svtkPComputeHistogram2DOutliers
  svtkPExtractHistogram2D
  svtkPPairwiseExtractHistogram2D
  svtkExtractPiece
  svtkMemoryLimitImageDataStreamer
  svtkTransmitImageDataPiece)

svtk_module_add_module(SVTK::FiltersParallelImaging
  CLASSES ${classes})
