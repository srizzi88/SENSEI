set(classes
  svtkCompassRepresentation
  svtkCompassWidget
  svtkGeoProjection
  svtkGeoTransform)

svtk_module_add_module(SVTK::GeovisCore
  CLASSES ${classes})
