set(classes
  svtkGeoJSONFeature
  svtkGeoJSONReader
  svtkGeoJSONWriter)

svtk_module_add_module(SVTK::IOGeoJSON
  CLASSES ${classes})
