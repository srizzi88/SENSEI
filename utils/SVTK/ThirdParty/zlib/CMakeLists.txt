svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtkzlib/zlib.h"
    VERSION       "1.2.11"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE ZLIB
    TARGETS ZLIB::ZLIB
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_zlib.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_zlib.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_zlib.h")
