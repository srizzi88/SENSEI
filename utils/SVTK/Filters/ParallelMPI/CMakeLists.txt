set(classes
  svtkDistributedPointCloudFilter
  svtkPExtractGrid
  svtkPExtractRectilinearGrid
  svtkPExtractVOI
  svtkStructuredImplicitConnectivity)

svtk_module_add_module(SVTK::FiltersParallelMPI
  CLASSES ${classes})
