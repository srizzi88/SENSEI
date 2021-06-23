svtk_add_test_mpi(svtkIOMPIImageCxxTests-MPI tests
  TESTING_DATA
  ParallelIso.cxx
  ParallelIso2.cxx
  )
svtk_test_cxx_executable(svtkIOMPIImageCxxTests-MPI tests)
