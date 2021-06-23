svtk_add_test_cxx(svtkIOParallelNetCDFCxxTests tests
  TestPNetCDFPOPReader.cxx
  )
svtk_test_cxx_executable(svtkIOParallelNetCDFCxxTests tests)

if(SVTK_USE_LARGE_DATA AND TARGET SVTK::ParallelMPI)
  svtk_add_test_mpi(svtkIOParallelNetCDFCxxTests-MPI tests
    TESTING_DATA
    PSLACReaderLinear.cxx
    PSLACReaderQuadratic.cxx
    )
  svtk_test_cxx_executable(svtkIOParallelNetCDFCxxTests-MPI tests)
endif()
