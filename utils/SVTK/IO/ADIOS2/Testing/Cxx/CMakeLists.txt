find_package(ADIOS2 2.4 REQUIRED)

if (ADIOS2_HAVE_MPI)
  if (svtk_io_adios2_core_enabled)
    set(TestADIOS2BPReaderSingleTimeStep_NUMPROCS 2)
  # For now svtkMultiBlockVolumeMapper does not support rendering in parallel
    set(TestADIOS2BPReaderMultiTimeSteps_NUMPROCS 2)
    set(TestADIOS2BPReaderMultiTimeSteps2D_NUMPROCS 2)
    svtk_add_test_mpi(svtkIOADIOS2CxxTests-MPI mpiTests TESTING_DATA
      TestADIOS2BPReaderMPISingleTimeStep.cxx
      TestADIOS2BPReaderMPIMultiTimeSteps3D.cxx,NO_VALID
      TestADIOS2BPReaderMPIMultiTimeSteps2D.cxx)
    svtk_test_cxx_executable(svtkIOADIOS2CxxTests-MPI mpiTests)
  endif()

  # VTX tests
  if (svtk_io_adios2_vtx_enabled)
    svtk_add_test_cxx(svtkIOADIOS2VTXCxxTests tests TESTING_DATA NO_OUTPUT
      UnitTestIOADIOS2VTX.cxx, NO_VALID
      TestIOADIOS2VTX_VTI3D.cxx,
      TestIOADIOS2VTX_VTI3DRendering.cxx, NO_VALID
      TestIOADIOS2VTX_VTU3D.cxx, NO_VALID
      TestIOADIOS2VTX_VTU3DRendering.cxx, NO_VALID
      TestIOADIOS2VTX_VTU2DRendering.cxx, NO_VALID
      TestIOADIOS2VTX_VTU1DRendering.cxx, NO_VALID)

    svtk_test_cxx_executable(svtkIOADIOS2VTXCxxTests tests)
    target_link_libraries(svtkIOADIOS2VTXCxxTests PUBLIC adios2::adios2)
  endif ()
else ()
  if (svtk_io_adios2_core_enabled)
    svtk_add_test_cxx(svtkIOADIOS2CxxTests tests TESTING_DATA
      TestADIOS2BPReaderSingleTimeStep.cxx
      TestADIOS2BPReaderMultiTimeSteps3D.cxx
      TestADIOS2BPReaderMultiTimeSteps2D.cxx)

    svtk_test_cxx_executable(svtkIOADIOS2CxxTests tests)
  endif ()
endif ()
