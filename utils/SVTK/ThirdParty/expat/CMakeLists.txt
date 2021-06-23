svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtkexpat/COPYING"
    VERSION       "2.2.6"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE EXPAT
    TARGETS EXPAT::EXPAT
    STANDARD_INCLUDE_DIRS)

include(svtkDetectLibraryType)
svtk_detect_library_shared(svtkexpat EXPAT::EXPAT)

if (svtkexpat_is_shared)
  set(XML_STATIC 0)
else ()
  set(XML_STATIC 1)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_expat.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_expat.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_expat.h")
