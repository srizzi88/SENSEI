set(classes
  svtkImageStencil
  svtkImageStencilToImage
  svtkImageToImageStencil
  svtkImplicitFunctionToImageStencil
  svtkLassoStencilSource
  svtkPolyDataToImageStencil
  svtkROIStencilSource)

svtk_module_add_module(SVTK::ImagingStencil
  CLASSES ${classes})
