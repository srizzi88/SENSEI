svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtknetcdf/COPYRIGHT"
    VERSION       "4.7.3"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE NetCDF
    TARGETS NetCDF::NetCDF
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_netcdf.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_netcdf.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_netcdf.h")
