set(classes
  svtkImageAccumulate
  svtkImageHistogram
  svtkImageHistogramStatistics)

svtk_module_add_module(SVTK::ImagingStatistics
  CLASSES ${classes})
