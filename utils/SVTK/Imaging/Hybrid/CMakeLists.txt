set(classes
  svtkBooleanTexture
  svtkCheckerboardSplatter
  svtkFastSplatter
  svtkGaussianSplatter
  svtkImageCursor3D
  svtkImageRectilinearWipe
  svtkImageToPoints
  svtkPointLoad
  svtkSampleFunction
  svtkShepardMethod
  svtkSliceCubes
  svtkSurfaceReconstructionFilter
  svtkTriangularTexture
  svtkVoxelModeller)

svtk_module_add_module(SVTK::ImagingHybrid
  CLASSES ${classes})
