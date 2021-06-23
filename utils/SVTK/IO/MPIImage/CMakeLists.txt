set(classes
  svtkPNrrdReader
  svtkMPIImageReader)

# Now to generate our object factory.
svtk_object_factory_declare(
  BASE      svtkNrrdReader
  OVERRIDE  svtkPNrrdReader)
svtk_object_factory_declare(
  BASE      svtkImageReader
  OVERRIDE  svtkMPIImageReader)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKIOMPIIMAGE_EXPORT")

svtk_module_add_module(SVTK::IOMPIImage
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
