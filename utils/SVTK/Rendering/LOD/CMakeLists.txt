set(classes
  svtkLODActor
  svtkQuadricLODActor)

svtk_module_add_module(SVTK::RenderingLOD
  CLASSES ${classes})
