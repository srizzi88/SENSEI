svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtkpng/LICENSE"
    VERSION       "1.6.37"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE PNG
    TARGETS PNG::PNG
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_png.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_png.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_png.h")
