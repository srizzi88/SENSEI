set(classes
  svtkThreadedImageWriter)

svtk_module_add_module(SVTK::IOAsynchronous
  CLASSES ${classes})
