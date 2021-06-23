if (TARGET SVTK::mpi)
  set (svtkFiltersParallelDIY2CxxTests-MPI_NUMPROCS 2)
  svtk_add_test_mpi(svtkFiltersParallelDIY2CxxTests-MPI tests
    TESTING_DATA
    TestPResampleToImageCompositeDataSet.cxx
    TestPResampleToImage.cxx
    TestPResampleWithDataSet2.cxx
    TestPResampleWithDataSet.cxx
    TestRedistributeDataSetFilter.cxx
  )

# We want 4 processes to test the svtkAggregateDataSetFilter
# properly.
  set (svtkFiltersParallelDIY2CxxTests-MPI_NUMPROCS 4)
  svtk_add_test_mpi(svtkFiltersParallelDIY2CxxTests-MPI no_data_tests_4_procs
    DIYAggregateDataSet.cxx
    TestAdaptiveResampleToImage.cxx
    TestGenerateGlobalIds.cxx
    )

  set(all_tests
    ${tests}
    ${no_data_tests_4_procs}
    )

  svtk_test_cxx_executable(svtkFiltersParallelDIY2CxxTests-MPI all_tests)
endif()

# non-mpi tests
svtk_add_test_cxx(svtkFiltersParallelDIY2CxxTests non_mpi_tests
  TestExtractSubsetWithSeed.cxx
  TestAdaptiveResampleToImage.cxx,NO_VALID
  TestGenerateGlobalIds.cxx,NO_VALID
  TestRedistributeDataSetFilter.cxx,NO_VALID)
svtk_test_cxx_executable(svtkFiltersParallelDIY2CxxTests non_mpi_tests)
