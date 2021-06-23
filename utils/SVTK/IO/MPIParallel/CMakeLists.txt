set(classes
  svtkPWindBladeReader
  svtkMPIMultiBlockPLOT3DReader)

svtk_object_factory_declare(
  BASE      svtkWindBladeReader
  OVERRIDE  svtkPWindBladeReader)
svtk_object_factory_declare(
  BASE      svtkMultiBlockPLOT3DReader
  OVERRIDE  svtkMPIMultiBlockPLOT3DReader)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKIOMPIPARALLEL_EXPORT")

svtk_module_add_module(SVTK::IOMPIParallel
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
