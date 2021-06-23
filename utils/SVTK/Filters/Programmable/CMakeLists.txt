set(classes
  svtkProgrammableAttributeDataFilter
  svtkProgrammableFilter
  svtkProgrammableGlyphFilter)

svtk_module_add_module(SVTK::FiltersProgrammable
  CLASSES ${classes})
