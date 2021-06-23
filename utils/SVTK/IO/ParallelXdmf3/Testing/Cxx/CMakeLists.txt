set(svtkIOParallelXdmf3CxxTests-MPI_NUMPROCS 2)
svtk_add_test_cxx(svtkIOParallelXdmf3CxxTests tests
  TestXdmf3Parallel.cxx,NO_VALID
  )

svtk_test_cxx_executable(svtkIOParallelXdmf3CxxTests tests)
