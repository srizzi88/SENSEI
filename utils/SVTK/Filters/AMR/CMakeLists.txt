set(classes
  svtkAMRCutPlane
  svtkAMRGaussianPulseSource
  svtkAMRResampleFilter
  svtkAMRSliceFilter
  svtkAMRToMultiBlockFilter
  svtkImageToAMR
  svtkParallelAMRUtilities)

svtk_module_add_module(SVTK::FiltersAMR
  CLASSES ${classes})
