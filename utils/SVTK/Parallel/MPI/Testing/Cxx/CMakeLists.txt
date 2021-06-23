svtk_add_test_mpi(svtkParallelMPICxxTests-MPI tests
  TESTING_DATA
  TestPProbe.cxx
  )
svtk_add_test_mpi(svtkParallelMPICxxTests-MPI no_data_tests
  #GenericCommunicator.cxx
  MPIController.cxx
  PDirectory.cxx
  PSystemTools.cxx
  )

set(svtkParallelMPICxxTests-MPI_NUMPROCS 2)
svtk_add_test_mpi(svtkParallelMPICxxTests-MPI 2_proc_tests
  TestNonBlockingCommunication.cxx
  TestProcess.cxx
  )

set(all_tests
  ${tests}
  ${no_data_tests}
  ${2_proc_tests}
  )
svtk_test_cxx_executable(svtkParallelMPICxxTests-MPI all_tests
  ExerciseMultiProcessController.cxx
  )
