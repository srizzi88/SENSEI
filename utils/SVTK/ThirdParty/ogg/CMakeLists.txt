svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtkogg/COPYING"
    VERSION       "1.3.4"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE OGG
    TARGETS OGG::OGG
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_ogg.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_ogg.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_ogg.h")
