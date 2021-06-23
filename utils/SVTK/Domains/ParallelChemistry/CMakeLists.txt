set(classes
  svtkPSimpleBondPerceiver)

svtk_object_factory_declare(
  BASE      svtkSimpleBondPerceiver
  OVERRIDE  svtkPSimpleBondPerceiver)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKDOMAINSPARALLELCHEMISTRY_EXPORT")

svtk_module_add_module(SVTK::DomainsParallelChemistry
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
