svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtkpugixml/LICENSE"
    VERSION       "1.10"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE pugixml
    TARGETS pugixml
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_pugixml.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_pugixml.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_pugixml.h")
