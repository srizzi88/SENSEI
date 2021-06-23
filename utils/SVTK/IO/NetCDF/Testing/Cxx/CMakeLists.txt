svtk_add_test_cxx(svtkIONetCDFCxxTests tests
  SLACMultipleModes.cxx
  SLACParticleReader.cxx
  SLACReaderLinear.cxx
  SLACReaderQuadratic.cxx
  TestMPASReader.cxx
  TestNetCDFCAMReader.cxx
  TestNetCDFPOPReader.cxx
  )

svtk_test_cxx_executable(svtkIONetCDFCxxTests tests)
