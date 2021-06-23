set(classes
  svtkImageAnisotropicDiffusion2D
  svtkImageAnisotropicDiffusion3D
  svtkImageCheckerboard
  svtkImageCityBlockDistance
  svtkImageConvolve
  svtkImageCorrelation
  svtkImageEuclideanDistance
  svtkImageEuclideanToPolar
  svtkImageGaussianSmooth
  svtkImageGradient
  svtkImageGradientMagnitude
  svtkImageHybridMedian2D
  svtkImageLaplacian
  svtkImageMedian3D
  svtkImageNormalize
  svtkImageRange3D
  svtkImageSeparableConvolution
  svtkImageSobel2D
  svtkImageSobel3D
  svtkImageSpatialAlgorithm
  svtkImageVariance3D
  svtkSimpleImageFilterExample
  svtkImageSlab
  svtkImageSlabReslice)

svtk_module_add_module(SVTK::ImagingGeneral
  CLASSES ${classes})
