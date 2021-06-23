svtk_module_third_party(
  INTERNAL
    HEADER_ONLY
    LICENSE_FILES "svtkpegtl/LICENSE"
    VERSION       "2.8.1"
  EXTERNAL
    PACKAGE PEGTL
    VERSION 2.0.0
    TARGETS PEGTL::PEGTL
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_pegtl.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_pegtl.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_pegtl.h")
