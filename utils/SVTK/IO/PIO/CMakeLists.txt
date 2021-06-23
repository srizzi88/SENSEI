set(classes
  BHTree
  PIOData
  PIOAdaptor
  svtkPIOReader)

svtk_module_add_module(SVTK::IOPIO
  CLASSES ${classes})
