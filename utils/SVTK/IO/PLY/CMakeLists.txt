set(classes
  svtkPLY
  svtkPLYReader
  svtkPLYWriter)

svtk_module_add_module(SVTK::IOPLY
  CLASSES ${classes})
