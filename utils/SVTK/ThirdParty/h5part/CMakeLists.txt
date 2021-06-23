# we don't support external h5part
svtk_module_third_party_internal(
  LICENSE_FILES "svtkh5part/COPYING"
  VERSION       "1.6.6"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_h5part.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_h5part.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_h5part.h")
