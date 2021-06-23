svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtklz4/lib/LICENSE"
    VERSION       "1.9.2"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE LZ4
    TARGETS LZ4::LZ4
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_lz4.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_lz4.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_lz4.h")
