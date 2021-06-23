set(classes
  svtkImageViewer
  svtkImageViewer2
  svtkResliceImageViewer
  svtkResliceImageViewerMeasurements
  )

svtk_module_add_module(SVTK::InteractionImage
  CLASSES ${classes})
