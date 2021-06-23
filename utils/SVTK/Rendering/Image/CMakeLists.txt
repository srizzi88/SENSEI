set(classes
  svtkDepthImageToPointCloud
  svtkImageResliceMapper
  svtkImageSliceCollection
  svtkImageStack)

svtk_module_add_module(SVTK::RenderingImage
  CLASSES ${classes})
