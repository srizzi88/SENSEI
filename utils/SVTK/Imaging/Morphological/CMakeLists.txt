set(classes
  svtkImageConnector
  svtkImageConnectivityFilter
  svtkImageContinuousDilate3D
  svtkImageContinuousErode3D
  svtkImageDilateErode3D
  svtkImageIslandRemoval2D
  svtkImageNonMaximumSuppression
  svtkImageOpenClose3D
  svtkImageSeedConnectivity
  svtkImageSkeleton2D
  svtkImageThresholdConnectivity)

svtk_module_add_module(SVTK::ImagingMorphological
  CLASSES ${classes})
