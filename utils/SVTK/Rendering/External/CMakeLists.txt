set(classes
  ExternalSVTKWidget
  svtkExternalLight
  svtkExternalOpenGLCamera
  svtkExternalOpenGLRenderWindow
  svtkExternalOpenGLRenderer)

svtk_module_add_module(SVTK::RenderingExternal
  CLASSES ${classes})
svtk_module_definitions(SVTK::RenderingExternal
  PRIVATE
    SVTK_OPENGL2)
