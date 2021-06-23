svtk_object_factory_declare(
  BASE      svtkResampleToImage
  OVERRIDE  svtkPResampleToImage)
svtk_object_factory_declare(
  BASE      svtkResampleWithDataSet
  OVERRIDE  svtkPResampleWithDataSet)

set(classes
  svtkAdaptiveResampleToImage
  svtkExtractSubsetWithSeed
  svtkGenerateGlobalIds
  svtkPResampleToImage
  svtkPResampleWithDataSet
  svtkRedistributeDataSetFilter)

if (TARGET SVTK::ParallelMPI)
  # FIXME: svtkDIYAggregateDataSetFilter should not require MPI
  # current implementation does (ref: paraview/paraview#19098)
  svtk_object_factory_declare(
    BASE      svtkAggregateDataSetFilter
    OVERRIDE  svtkDIYAggregateDataSetFilter)
  list(APPEND classes
    svtkDIYAggregateDataSetFilter)
endif()

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKFILTERSPARALLELDIY2_EXPORT")

svtk_module_add_module(SVTK::FiltersParallelDIY2
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source} svtkDIYKdTreeUtilities.cxx
  PRIVATE_HEADERS ${svtk_object_factory_header} svtkDIYKdTreeUtilities.h)
