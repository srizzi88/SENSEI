svtk_object_factory_declare(
  BASE      svtkTextRenderer
  OVERRIDE  svtkMathTextFreeTypeTextRenderer)
svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKRENDERINGFREETYPE_EXPORT")

set(classes
  svtkFreeTypeStringToImage
  svtkFreeTypeTools
  svtkMathTextFreeTypeTextRenderer
  svtkMathTextUtilities
  svtkScaledTextActor
  svtkTextRendererStringToImage
  svtkVectorText)

set(sources
  ${svtk_object_factory_source}

  fonts/face_arial_bold.cxx
  fonts/face_arial_bold_italic.cxx
  fonts/face_arial.cxx
  fonts/face_arial_italic.cxx
  fonts/face_courier_bold.cxx
  fonts/face_courier_bold_italic.cxx
  fonts/face_courier.cxx
  fonts/face_courier_italic.cxx
  fonts/face_times_bold.cxx
  fonts/face_times_bold_italic.cxx
  fonts/face_times.cxx
  fonts/face_times_italic.cxx)

set(private_headers
  ${svtk_object_factory_header})

svtk_module_add_module(SVTK::RenderingFreeType
  CLASSES         ${classes}
  SOURCES         ${sources}
  PRIVATE_HEADERS ${private_headers})
