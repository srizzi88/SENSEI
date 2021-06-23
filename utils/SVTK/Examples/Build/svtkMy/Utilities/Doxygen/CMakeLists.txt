#
# Build the documentation
#

include(svtkMyDocumentation)

if(BUILD_DOCUMENTATION)

  #
  # The needed perl scripts are in Utilities/Doxygen in SVTK_SOURCE_DIR
  #
  if(NOT DEFINED SVTK_SOURCE_DIR)
    # go up three directories from svtkMy source directory
    get_filename_component(_tmp "${SVTKMY_SOURCE_DIR}" DIRECTORY)
    get_filename_component(_tmp "${_tmp}" DIRECTORY)
    get_filename_component(_tmp "${_tmp}" DIRECTORY)
    set(SVTK_SOURCE_DIR "${_tmp}" CACHE PATH "SVTK Source Directory")
  endif()

  option(DOCUMENTATION_DOWNLOAD_SVTK_TAGFILE "Download the SVTK tag file." ON)
  mark_as_advanced(DOCUMENTATION_DOWNLOAD_SVTK_TAGFILE)

  #
  # Configure the script and the doxyfile, then add target
  #
  if(DOT AND NOT DOT_PATH)
    get_filename_component(DOT_PATH "${DOT}" DIRECTORY)
  endif()

  configure_file(
    "${SVTKMY_SOURCE_DIR}/Utilities/Doxygen/doxyfile.in"
    "${SVTKMY_BINARY_DIR}/Utilities/Doxygen/doxyfile")

  configure_file(
    "${SVTKMY_SOURCE_DIR}/Utilities/Doxygen/doc_makeall.sh.in"
    "${SVTKMY_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh")

  add_custom_target(svtkMyDoxygenDoc
    "${BASH}"
    "${SVTKMY_BINARY_DIR}/Utilities/Doxygen/doc_makeall.sh")

endif()
