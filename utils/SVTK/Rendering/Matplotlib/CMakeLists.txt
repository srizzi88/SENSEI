set(classes
  svtkMatplotlibMathTextUtilities)

svtk_object_factory_declare(
  BASE      svtkMathTextUtilities
  OVERRIDE  svtkMatplotlibMathTextUtilities)
svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKRENDERINGMATPLOTLIB_EXPORT")

svtk_module_add_module(SVTK::RenderingMatplotlib
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})

set_property(GLOBAL APPEND
  PROPERTY
    svtk_required_python_modules "matplotlib>=2.0.0")
