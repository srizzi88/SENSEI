svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtklibxml2/Copyright"
    VERSION       "2.9.10"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE LibXml2
    TARGETS LibXml2::LibXml2
    STANDARD_INCLUDE_DIRS)

include(svtkDetectLibraryType)
svtk_detect_library_shared(svtklibxml2 LibXml2::LibXml2)

if (svtklibxml2_is_shared)
  set(LIBXML_STATIC 0)
else ()
  set(LIBXML_STATIC 1)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_libxml2.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_libxml2.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_libxml2.h")
