svtk_module_third_party_internal(
    LICENSE_FILES "svtkloguru/LICENSE"
    VERSION       "2.0"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_loguru.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_loguru.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_loguru.h")
