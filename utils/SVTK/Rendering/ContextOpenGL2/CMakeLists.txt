set(classes
  svtkOpenGLContextActor
  svtkOpenGLContextBufferId
  svtkOpenGLContextDevice2D
  svtkOpenGLContextDevice3D
  svtkOpenGLPropItem)

set(opengl_overrides
  ContextActor
  ContextDevice2D
  ContextDevice3D
  PropItem)
foreach (opengl_override IN LISTS opengl_overrides)
  svtk_object_factory_declare(
    BASE "svtk${opengl_override}"
    OVERRIDE "svtkOpenGL${opengl_override}")
endforeach ()
svtk_object_factory_declare(
  BASE svtkAbstractContextBufferId
  OVERRIDE svtkOpenGLContextBufferId)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKRENDERINGCONTEXTOPENGL2_EXPORT")

svtk_module_add_module(SVTK::RenderingContextOpenGL2
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
