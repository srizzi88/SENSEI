set(classes
  svtkBlockSelector
  svtkConvertSelection
  svtkExpandMarkedElements
  svtkExtractBlock
  svtkExtractCells
  svtkExtractCellsByType
  svtkExtractDataArraysOverTime
  svtkExtractDataOverTime
  svtkExtractDataSets
  svtkExtractEdges
  svtkExtractExodusGlobalTemporalVariables
  svtkExtractGeometry
  svtkExtractGrid
  svtkExtractLevel
  svtkExtractPolyDataGeometry
  svtkExtractRectilinearGrid
  svtkExtractSelectedArraysOverTime
  svtkExtractSelectedBlock
  svtkExtractSelectedIds
  svtkExtractSelectedLocations
  svtkExtractSelectedPolyDataIds
  svtkExtractSelectedRows
  svtkExtractSelectedThresholds
  svtkExtractSelection
  svtkExtractTensorComponents
  svtkExtractTimeSteps
  svtkExtractUnstructuredGrid
  svtkExtractVectorComponents
  svtkFrustumSelector
  svtkHierarchicalDataExtractDataSets
  svtkHierarchicalDataExtractLevel
  svtkLocationSelector
  svtkProbeSelectedLocations
  svtkSelector
  svtkValueSelector)

if (NOT SVTK_LEGACY_REMOVE)
  list(APPEND classes
    svtkExtractTemporalFieldData)
endif()

svtk_module_add_module(SVTK::FiltersExtraction
  CLASSES ${classes})
