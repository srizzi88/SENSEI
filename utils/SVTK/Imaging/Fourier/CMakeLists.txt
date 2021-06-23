set(classes
  svtkImageButterworthHighPass
  svtkImageButterworthLowPass
  svtkImageFFT
  svtkImageFourierCenter
  svtkImageFourierFilter
  svtkImageIdealHighPass
  svtkImageIdealLowPass
  svtkImageRFFT
  svtkTableFFT)

svtk_module_add_module(SVTK::ImagingFourier
  CLASSES ${classes})
