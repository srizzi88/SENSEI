set(classes
  svtkGenericClip
  svtkGenericContourFilter
  svtkGenericCutter
  svtkGenericDataSetTessellator
  svtkGenericGeometryFilter
  svtkGenericGlyph3DFilter
  svtkGenericOutlineFilter
  svtkGenericProbeFilter
  svtkGenericStreamTracer)

svtk_module_add_module(SVTK::FiltersGeneric
  CLASSES ${classes})
