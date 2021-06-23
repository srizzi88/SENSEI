svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtkglew/LICENSE.txt"
    VERSION       "2.1.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE GLEW
    TARGETS GLEW::GLEW
    STANDARD_INCLUDE_DIRS)

if(SVTK_OPENGL_USE_GLES OR ANDROID AND NOT SVTK_IOS_BUILD)
  set(SVTK_MODULE_svtkglew_GLES3 1)
endif()

include(svtkDetectLibraryType)
svtk_detect_library_shared(svtkglew GLEW::GLEW)

set(SVTK_GLEW_SHARED "${svtkglew_is_shared}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_glew.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_glew.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_glew.h")
