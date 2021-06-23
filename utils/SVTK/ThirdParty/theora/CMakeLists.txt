svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtktheora/COPYING"
    VERSION       "1.1.1"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE THEORA
    TARGETS THEORA::THEORA
            THEORA::DEC
            THEORA::ENC
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_theora.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_theora.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_theora.h")
