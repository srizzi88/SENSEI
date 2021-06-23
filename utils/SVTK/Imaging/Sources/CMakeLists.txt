set(classes
  svtkImageCanvasSource2D
  svtkImageEllipsoidSource
  svtkImageGaussianSource
  svtkImageGridSource
  svtkImageMandelbrotSource
  svtkImageNoiseSource
  svtkImageSinusoidSource)

svtk_module_add_module(SVTK::ImagingSources
  CLASSES ${classes})
