set(classes
  svtkPExodusIIReader
  svtkPExodusIIWriter)

svtk_object_factory_declare(
  BASE      svtkExodusIIWriter
  OVERRIDE  svtkPExodusIIWriter)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKIOPARALLELEXODUS_EXPORT")

svtk_module_add_module(SVTK::IOParallelExodus
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
