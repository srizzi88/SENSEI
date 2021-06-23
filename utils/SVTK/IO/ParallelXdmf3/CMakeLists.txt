set(classes
  svtkPXdmf3Writer)

svtk_object_factory_declare(
  BASE svtkXdmf3Writer
  OVERRIDE svtkPXdmf3Writer)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKIOPARALLELXDMF3_EXPORT")

svtk_module_add_module(SVTK::IOParallelXdmf3
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
