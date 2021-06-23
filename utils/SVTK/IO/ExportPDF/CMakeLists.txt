set(classes
  svtkPDFContextDevice2D
  svtkPDFExporter)

svtk_object_factory_declare(
  BASE svtkContextDevice2D
  OVERRIDE svtkPDFContextDevice2D)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKIOEXPORTPDF_EXPORT")

svtk_module_add_module(SVTK::IOExportPDF
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
