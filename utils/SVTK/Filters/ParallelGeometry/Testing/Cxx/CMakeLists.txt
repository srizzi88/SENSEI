set(svtkFiltersParallelGeometryCxxTests-MPI_NUMPROCS 4)
set(Tests_SRCS
  TestPStructuredGridConnectivity.cxx
  TestPStructuredGridGhostDataGenerator.cxx
  TestPUnstructuredGridGhostCellsGenerator.cxx
  TestPUniformGridGhostDataGenerator.cxx)

svtk_add_test_mpi(svtkFiltersParallelGeometryCxxTests-MPI tests ${Tests_SRCS})

set(svtkFiltersParallelGeometryCxxTests-MPI_NUMPROCS 1)
svtk_add_test_mpi(svtkFiltersParallelGeometryCxxTests-MPI data_tests_1_proc
  ParallelConnectivity1,ParallelConnectivity.cxx,TESTING_DATA,NO_VALID
  )

set(svtkFiltersParallelGeometryCxxTests-MPI_NUMPROCS 4)
svtk_add_test_mpi(svtkFiltersParallelGeometryCxxTests-MPI data_tests_4_procs
  ParallelConnectivity4,ParallelConnectivity.cxx,TESTING_DATA,NO_VALID
  )

svtk_add_test_mpi(svtkFiltersParallelGeometryCxxTests-MPI data_tests_4_procs
  TestPolyhedralMeshDistributedDataFilter.cxx,TESTING_DATA,NO_VALID
)

set(all_tests
  ${tests}
  ${data_tests_1_proc}
  ${data_tests_4_procs}
)

svtk_test_cxx_executable(svtkFiltersParallelGeometryCxxTests-MPI all_tests
  UnstructuredGhostZonesCommon.cxx)
