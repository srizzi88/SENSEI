set(classes
  svtkPNetCDFPOPReader
  svtkPSLACReader)

svtk_object_factory_declare(
  BASE      svtkSLACReader
  OVERRIDE  svtkPSLACReader)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKIOPARALLELNETCDF_EXPORT")

svtk_module_add_module(SVTK::IOParallelNetCDF
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
