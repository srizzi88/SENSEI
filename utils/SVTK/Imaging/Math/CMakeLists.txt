set(classes
  svtkImageDivergence
  svtkImageDotProduct
  svtkImageLogarithmicScale
  svtkImageLogic
  svtkImageMagnitude
  svtkImageMaskBits
  svtkImageMathematics
  svtkImageWeightedSum)

svtk_module_add_module(SVTK::ImagingMath
  CLASSES ${classes})
