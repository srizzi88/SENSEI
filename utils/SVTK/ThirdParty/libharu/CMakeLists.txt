svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtklibharu/LICENCE"
    VERSION       "2.4.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE LibHaru
    # Unreleased. Requires these PRs:
    # https://github.com/libharu/libharu/pull/157
    # https://github.com/libharu/libharu/pull/187
    VERSION 2.4.0
    TARGETS LibHaru::LibHaru
    STANDARD_INCLUDE_DIRS)

include(svtkDetectLibraryType)
svtk_detect_library_shared(svtklibharu LibHaru::LibHaru)

set(HPDF_DLL 0)
if (WIN32 AND svtklibharu_is_shared)
  set(HPDF_DLL 1)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_libharu.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_libharu.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_libharu.h")
