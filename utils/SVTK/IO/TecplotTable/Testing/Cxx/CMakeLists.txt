svtk_add_test_cxx(svtkIOTecplotTableCxxTests tests
  NO_VALID
  TestTecplotTableReader.cxx
  )

svtk_test_cxx_executable(svtkIOTecplotTableCxxTests tests)
