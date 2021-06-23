svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtklzma/COPYING"
    VERSION       "5.2.4"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE LZMA
    TARGETS LZMA::LZMA
    STANDARD_INCLUDE_DIRS)

include(svtkDetectLibraryType)
svtk_detect_library_shared(svtklzma LZMA::LZMA)

set(LZMA_BUILT_AS_DYNAMIC_LIB "${svtklzma_is_shared}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_lzma.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_lzma.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_lzma.h")
