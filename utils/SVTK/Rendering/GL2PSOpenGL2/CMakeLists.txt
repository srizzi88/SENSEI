svtk_object_factory_declare(
  BASE svtkOpenGLGL2PSHelper
  OVERRIDE svtkOpenGLGL2PSHelperImpl)
svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKRENDERINGGL2PSOPENGL2_EXPORT")

set(classes
  svtkOpenGLGL2PSHelperImpl)

svtk_module_add_module(SVTK::RenderingGL2PSOpenGL2
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
