svtk_add_test_cxx(svtkRenderingParallelCxxTests tests
  PrmMagnify.cxx
  )
svtk_test_cxx_executable(svtkRenderingParallelCxxTests tests)

if(TARGET SVTK::ParallelMPI)
  set(svtkRenderingParallelCxxTests-MPI_NUMPROCS 2)
  svtk_add_test_mpi(svtkRenderingParallelCxxTests-MPI tests
    TestSimplePCompositeZPass.cxx,TESTING_DATA
    TestParallelRendering.cxx,TESTING_DATA
    )
endif()

set(all_tests
  ${tests}
  ${no_data_tests}
  )
svtk_test_cxx_executable(svtkRenderingParallelCxxTests-MPI all_tests)

if (PYTHON_EXECUTABLE)
  svtk_module_test_executable(
    svtkRenderingParallel-TestClientServerRendering TestClientServerRendering.cxx)

  ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
    NAME svtkRenderingParallel-TestClientServerRendering
    COMMAND ${PYTHON_EXECUTABLE}
      ${SVTK_SOURCE_DIR}/CMake/svtkTestDriver.py
      --process
        $<TARGET_FILE:svtkRenderingParallel-TestClientServerRendering>
        --port 12367
        --server
      -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
        --process
        $<TARGET_FILE:svtkRenderingParallel-TestClientServerRendering>
        --port 12367
        -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
        -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
        -V "DATA{../Data/Baseline/TestClientServerRendering.png,:}"
  )
endif()
