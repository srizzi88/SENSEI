set(classes
  svtkGL2PSExporter
  svtkOpenGLGL2PSExporter
  )

svtk_object_factory_declare(
  BASE svtkGL2PSExporter
  OVERRIDE svtkOpenGLGL2PSExporter)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKIOEXPORTGL2PS_EXPORT")

svtk_module_add_module(SVTK::IOExportGL2PS
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
