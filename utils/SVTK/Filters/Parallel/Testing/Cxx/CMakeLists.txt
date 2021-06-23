svtk_add_test_cxx(svtkFiltersParallelCxxTests testsStd
  TestAngularPeriodicFilter.cxx
  )
svtk_test_cxx_executable(svtkFiltersParallelCxxTests testsStd)

if (TARGET SVTK::ParallelMPI)
  set(svtkFiltersParallelCxxTests-MPI_NUMPROCS 2)
  svtk_add_test_mpi(svtkFiltersParallelCxxTests-MPI tests
    TESTING_DATA
    DistributedData.cxx
    DistributedDataRenderPass.cxx
    PTextureMapToSphere.cxx
    TestPExtractDataArraysOverTime.cxx,NO_VALID
    TransmitImageData.cxx
    TransmitImageDataRenderPass.cxx
    TransmitRectilinearGrid.cxx
    TransmitStructuredGrid.cxx
    )
  svtk_add_test_mpi(svtkFiltersParallelCxxTests-MPI no_data_tests
    ParallelResampling.cxx
    PCellDataToPointData.cxx
    UnitTestPMaskPoints.cxx
    )

  # We want 4 processes to test the svtkAggregateDataSetFilter
  # properly
  set(svtkFiltersParallelCxxTests-MPI_NUMPROCS 4)
  svtk_add_test_mpi(svtkFiltersParallelCxxTests-MPI no_data_tests_4_procs
    AggregateDataSet.cxx
    )


  set(all_tests
    ${tests}
    ${no_data_tests}
    ${no_data_tests_4_procs}
    )
  svtk_test_cxx_executable(svtkFiltersParallelCxxTests-MPI all_tests)
endif()
