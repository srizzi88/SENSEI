set(classes
  svtkPAutoCorrelativeStatistics
  svtkPContingencyStatistics
  svtkPCorrelativeStatistics
  svtkPDescriptiveStatistics
  svtkPKMeansStatistics
  svtkPMultiCorrelativeStatistics
  svtkPOrderStatistics
  svtkPPCAStatistics
  svtkPBivariateLinearTableThreshold)

svtk_module_add_module(SVTK::FiltersParallelStatistics
  CLASSES ${classes})
