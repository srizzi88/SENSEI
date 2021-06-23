svtk_add_test_mpi(svtkFiltersParallelMPICxxTests-MPI tests
  TestImplicitConnectivity.cxx
  )

set(svtkFiltersParallelMPICxxTests-MPI_NUMPROCS 1)
svtk_add_test_mpi(svtkFiltersParallelMPICxxTests-MPI test_1_procs
  TestDistributedPointCloudFilter1,TestDistributedPointCloudFilter.cxx
  )

set(svtkFiltersParallelMPICxxTests-MPI_NUMPROCS 2)
svtk_add_test_mpi(svtkFiltersParallelMPICxxTests-MPI test_2_procs
  TestDistributedPointCloudFilter2,TestDistributedPointCloudFilter.cxx
  )

set(svtkFiltersParallelMPICxxTests-MPI_NUMPROCS 5)
svtk_add_test_mpi(svtkFiltersParallelMPICxxTests-MPI test_5_procs
  TestDistributedPointCloudFilter5,TestDistributedPointCloudFilter.cxx
  )

set(all_tests
  ${tests}
  ${test_4_procs}
  ${test_1_procs}
  ${test_2_procs}
  ${test_5_procs}
  )

svtk_test_cxx_executable(svtkFiltersParallelMPICxxTests-MPI all_tests)
