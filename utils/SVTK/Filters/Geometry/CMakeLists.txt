set(classes
  svtkAbstractGridConnectivity
  svtkCompositeDataGeometryFilter
  svtkDataSetGhostGenerator
  svtkDataSetRegionSurfaceFilter
  svtkDataSetSurfaceFilter
  svtkExplicitStructuredGridSurfaceFilter
  svtkGeometryFilter
  svtkHierarchicalDataSetGeometryFilter
  svtkImageDataGeometryFilter
  svtkImageDataToUniformGrid
  svtkLinearToQuadraticCellsFilter
  svtkProjectSphereFilter
  svtkRectilinearGridGeometryFilter
  svtkRectilinearGridPartitioner
  svtkStructuredAMRGridConnectivity
  svtkStructuredAMRNeighbor
  svtkStructuredGridConnectivity
  svtkStructuredGridGeometryFilter
  svtkStructuredGridGhostDataGenerator
  svtkStructuredGridPartitioner
  svtkStructuredNeighbor
  svtkStructuredPointsGeometryFilter
  svtkUniformGridGhostDataGenerator
  svtkUnstructuredGridGeometryFilter)

svtk_module_add_module(SVTK::FiltersGeometry
  CLASSES ${classes})
