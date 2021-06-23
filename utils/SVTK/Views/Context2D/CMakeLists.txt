set(classes
  svtkContextView
  svtkContextInteractorStyle
  )

svtk_module_add_module(SVTK::ViewsContext2D
  CLASSES ${classes})
