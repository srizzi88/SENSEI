svtk_add_test_cxx(svtkFiltersFlowPathsCxxTests tests
  TestBSPTree.cxx
  TestEvenlySpacedStreamlines2D.cxx
  TestStreamTracer.cxx,NO_VALID
  TestStreamTracerSurface.cxx
  TestAMRInterpolatedVelocityField.cxx,NO_VALID
  TestParticleTracers.cxx,NO_VALID
  TestLagrangianIntegrationModel.cxx,NO_VALID
  TestLagrangianParticle.cxx,NO_VALID
  TestLagrangianParticleTracker.cxx
  )
svtk_test_cxx_executable(svtkFiltersFlowPathsCxxTests tests
  RENDERING_FACTORY
  )
