svtk_object_factory_declare(
  BASE svtkFreeTypeTools
  OVERRIDE svtkFontConfigFreeTypeTools)

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKRENDERINGFREETYPEFONTCONFIG_EXPORT")

set(classes
  svtkFontConfigFreeTypeTools)

svtk_module_add_module(SVTK::RenderingFreeTypeFontConfig
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})

svtk_module_find_package(PACKAGE FontConfig)
svtk_module_link(SVTK::RenderingFreeTypeFontConfig
  PRIVATE
    FontConfig::FontConfig)
