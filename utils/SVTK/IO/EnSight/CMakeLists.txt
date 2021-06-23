set(classes
  svtkEnSight6BinaryReader
  svtkEnSight6Reader
  svtkEnSightGoldBinaryReader
  svtkEnSightGoldReader
  svtkEnSightMasterServerReader
  svtkEnSightReader
  svtkGenericEnSightReader)

svtk_module_add_module(SVTK::IOEnSight
  CLASSES ${classes})
