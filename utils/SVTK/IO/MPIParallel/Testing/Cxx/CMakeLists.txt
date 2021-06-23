set(_known_little_endian FALSE)
if (DEFINED SVTK_WORDS_BIGENDIAN)
  if (NOT SVTK_WORDS_BIGENDIAN)
    set(_known_little_endian TRUE)
  endif()
endif()

if (enable_large_data_tests)
  set(TestPWindBladeReader_NUMPROCS 1)
  svtk_add_test_mpi(svtkIOMPIParallelCxxTests-MPI tests
    TESTING_DATA
    TestPWindBladeReader.cxx
    )
  svtk_test_cxx_executable(svtkIOMPIParallelCxxTests-MPI tests)
endif()
