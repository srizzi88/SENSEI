set(TestPLagrangianParticleTracker_NUMPROCS 2)
set(TestPStream_NUMPROCS 2)
set(TestPStreamGeometry_NUMPROCS 4)
set(TestPParticleTracers_NUMPROCS 2)
set(TestPStreamAMR_NUMPROCS 4)
svtk_add_test_mpi(svtkFiltersParallelFlowPathsCxxTests-MPI tests
  TestPLagrangianParticleTracker.cxx,TESTING_DATA
  TestPStream.cxx,TESTING_DATA
  TestPStreamGeometry.cxx
  TestPParticleTracers.cxx
  TestPStreamAMR.cxx,TESTING_DATA,NO_VALID
  )
svtk_test_cxx_executable(svtkFiltersParallelFlowPathsCxxTests-MPI tests
  TestVectorFieldSource.cxx)
