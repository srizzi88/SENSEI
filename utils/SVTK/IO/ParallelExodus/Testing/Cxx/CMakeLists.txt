svtk_add_test_cxx(svtkIOParallelExodusCxxTests tests
  NO_VALID
  TestExodusImplicitArrays.cxx
  )
svtk_test_cxx_executable(svtkIOParallelExodusCxxTests tests)
