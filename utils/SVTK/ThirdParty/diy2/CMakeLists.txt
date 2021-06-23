svtk_module_third_party_internal(
  HEADER_ONLY
  LICENSE_FILES "svtkdiy2/LICENSE.txt"
                "svtkdiy2/LEGAL.txt"
  VERSION       "3.5.0")

if (TARGET SVTK::mpi)
  svtk_module_link(SVTK::diy2
    INTERFACE
      SVTK::mpi)

  set(DIY_NO_MPI FALSE)
else ()
  set(DIY_NO_MPI TRUE)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_diy2.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_diy2.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_diy2.h")
