svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtkfreetype/docs/FTL.TXT"
                  "svtkfreetype/docs/LICENSE.TXT"
    VERSION       "2.10.1"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Freetype
    TARGETS Freetype::Freetype
    STANDARD_INCLUDE_DIRS)

include(svtkDetectLibraryType)
svtk_detect_library_shared(svtkfreetype Freetype::Freetype)

set(SVTK_FREETYPE_SHARED "${svtkfreetype_is_shared}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_freetype.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_freetype.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_freetype.h")
