svtk_add_test_cxx(svtkIOLSDynaCxxTests tests
  TestLSDynaReader.cxx
  TestLSDynaReaderDeflection.cxx
  #TestLSDynaReaderNoDefl.cxx
  TestLSDynaReaderSPH.cxx
  )

svtk_test_cxx_executable(svtkIOLSDynaCxxTests tests)
