set(classes
  svtkConvertSelectionDomain
  svtkDataRepresentation
  svtkEmptyRepresentation
  svtkRenderViewBase
  svtkView
  svtkViewTheme
  )

svtk_module_add_module(SVTK::ViewsCore
  CLASSES ${classes})
