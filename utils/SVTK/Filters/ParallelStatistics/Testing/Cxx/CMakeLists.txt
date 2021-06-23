svtk_add_test_mpi(svtkFiltersParallelStatisticsCxxTests-MPI tests
  TestRandomPContingencyStatisticsMPI.cxx
  TestRandomPKMeansStatisticsMPI.cxx
  TestRandomPMomentStatisticsMPI.cxx
  TestRandomPOrderStatisticsMPI.cxx
  # # The following file was being compiled but never added as a testin older
  # # svtk. Why is that so?
  # TestRealDataPDescriptiveStatisticsMPI.cxx
  )
svtk_test_cxx_executable(svtkFiltersParallelStatisticsCxxTests-MPI tests)
