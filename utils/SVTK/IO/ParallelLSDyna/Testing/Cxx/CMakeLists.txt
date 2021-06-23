
set(PLSDynaReader_NUMPROCS 2)
svtk_add_test_mpi(svtkIOParallelLSDynaCxxTests-MPI tests
  TESTING_DATA
  PLSDynaReader.cxx
  )
svtk_test_cxx_executable(svtkIOParallelLSDynaCxxTests-MPI tests)
