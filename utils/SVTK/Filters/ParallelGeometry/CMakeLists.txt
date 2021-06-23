svtk_object_factory_declare(
  BASE      svtkConnectivityFilter
  OVERRIDE  svtkPConnectivityFilter)
svtk_object_factory_declare(
  BASE      svtkDistributedDataFilter
  OVERRIDE  svtkPDistributedDataFilter)
svtk_object_factory_declare(
  BASE      svtkUnstructuredGridGhostCellsGenerator
  OVERRIDE  svtkPUnstructuredGridGhostCellsGenerator)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKFILTERSPARALLELGEOMETRY_EXPORT")

set(classes
  svtkPConnectivityFilter
  svtkPDataSetGhostGenerator
  svtkPDistributedDataFilter
  svtkPStructuredGridConnectivity
  svtkPStructuredGridGhostDataGenerator
  svtkPUniformGridGhostDataGenerator
  svtkPUnstructuredGridGhostCellsGenerator)

svtk_module_add_module(SVTK::FiltersParallelGeometry
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
