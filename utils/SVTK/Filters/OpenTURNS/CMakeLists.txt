find_package(OpenTURNS 1.8 REQUIRED)
# XXX(fixme)
include(${OPENTURNS_USE_FILE})

set(classes
  svtkOTDensityMap
  svtkOTFilter
  svtkOTKernelSmoothing
  svtkOTScatterPlotMatrix
  svtkOTUtilities)

svtk_object_factory_declare(
  BASE svtkScatterPlotMatrix
  OVERRIDE svtkOTScatterPlotMatrix)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKFILTERSOPENTURNS_EXPORT")

svtk_module_add_module(SVTK::FiltersOpenTURNS
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})

svtk_module_link(SVTK::FiltersOpenTURNS
  PRIVATE
    ${OPENTURNS_LIBRARIES})
svtk_module_include(SVTK::FiltersOpenTURNS
  PRIVATE
    ${OPENTURNS_INCLUDE_DIRS})
