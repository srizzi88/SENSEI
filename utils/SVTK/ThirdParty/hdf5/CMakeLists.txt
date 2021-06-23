if (POLICY CMP0074)
  cmake_policy(PUSH)
  cmake_policy(SET CMP0074 NEW)
endif()

set(HDF5_EXTERNALLY_CONFIGURED 1)

svtk_module_third_party(
  INTERNAL
    LICENSE_FILES "svtkhdf5/COPYING"
                  "svtkhdf5/COPYING_LBNL_HDF5"
    VERSION       "1.10.6"
    STANDARD_INCLUDE_DIRS
    INTERFACE
  EXTERNAL
    PACKAGE       HDF5
    COMPONENTS    C HL
    TARGETS       hdf5::hdf5
                  hdf5::hdf5_hl
    USE_VARIABLES HDF5_IS_PARALLEL
    STANDARD_INCLUDE_DIRS)

set(svtkhdf5_is_parallel 0)
if (SVTK_MODULE_USE_EXTERNAL_svtkhdf5 AND HDF5_IS_PARALLEL)
  if (NOT TARGET SVTK::mpi)
    message(FATAL_ERROR
      "An external MPI-aware HDF5 requires that SVTK be built with MPI support "
      "as well.")
  endif ()

  set(svtkhdf5_is_parallel 1)
  svtk_module_link(SVTK::hdf5
    INTERFACE
      SVTK::mpi)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtk_hdf5.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtk_hdf5.h")

svtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/svtk_hdf5.h")

if (POLICY CMP0074)
  cmake_policy(POP)
endif()
