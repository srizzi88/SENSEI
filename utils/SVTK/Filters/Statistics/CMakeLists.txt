set(classes
  svtkAutoCorrelativeStatistics
  svtkBivariateLinearTableThreshold
  svtkComputeQuartiles
  svtkContingencyStatistics
  svtkCorrelativeStatistics
  svtkDescriptiveStatistics
  svtkHighestDensityRegionsStatistics
  svtkExtractFunctionalBagPlot
  svtkKMeansDistanceFunctorCalculator
  svtkKMeansDistanceFunctor
  svtkKMeansStatistics
  svtkMultiCorrelativeStatistics
  svtkOrderStatistics
  svtkPCAStatistics
  svtkStatisticsAlgorithm
  svtkStrahlerMetric
  svtkStreamingStatistics)

set(headers
  svtkStatisticsAlgorithmPrivate.h)

svtk_module_add_module(SVTK::FiltersStatistics
  CLASSES ${classes}
  HEADERS ${headers})
