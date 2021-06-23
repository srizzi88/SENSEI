set(classes
  svtkImplicitTextureCoords
  svtkScalarsToTextureFilter
  svtkTextureMapToCylinder
  svtkTextureMapToPlane
  svtkTextureMapToSphere
  svtkThresholdTextureCoords
  svtkTransformTextureCoords
  svtkTriangularTCoords)

svtk_module_add_module(SVTK::FiltersTexture
  CLASSES ${classes})
