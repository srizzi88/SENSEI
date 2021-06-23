set(classes
  svtkH5PartReader)

svtk_module_add_module(SVTK::IOH5part
  CLASSES ${classes})

svtk_module_definitions(SVTK::IOH5part
  PRIVATE
    H5_USE_16_API)
