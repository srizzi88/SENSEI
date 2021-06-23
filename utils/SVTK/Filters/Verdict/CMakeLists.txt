set(classes
  svtkCellQuality
  svtkCellSizeFilter
  svtkMatrixMathFilter
  svtkMeshQuality)

svtk_module_add_module(SVTK::FiltersVerdict
  CLASSES ${classes})
