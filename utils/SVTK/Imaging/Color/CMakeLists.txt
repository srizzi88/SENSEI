set(classes
  svtkImageHSIToRGB
  svtkImageHSVToRGB
  svtkImageLuminance
  svtkImageMapToRGBA
  svtkImageMapToWindowLevelColors
  svtkImageQuantizeRGBToIndex
  svtkImageRGBToHSI
  svtkImageRGBToHSV
  svtkImageRGBToYIQ
  svtkImageYIQToRGB)

svtk_module_add_module(SVTK::ImagingColor
  CLASSES ${classes})
