set(classes
  svtkKdTreeSelector
  svtkLinearSelector
  svtkCellDistanceSelector)

svtk_module_add_module(SVTK::FiltersSelection
  CLASSES ${classes})
