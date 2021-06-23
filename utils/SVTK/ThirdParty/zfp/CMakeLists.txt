svtk_module_third_party_internal(
  LICENSE_FILES "svtkzfp/LICENSE"
  VERSION       "0.5.5"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_zfp.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_zfp.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_zfp.h")
