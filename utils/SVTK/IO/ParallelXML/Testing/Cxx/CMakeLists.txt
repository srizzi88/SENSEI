# note, to enable testing the svtkParallelMPI module should be enabled

svtk_add_test_cxx(svtkIOParallelXMLCxxTests tests
  TestParallelUnstructuredGridIO.cxx,No_DATA,NO_VALID
  )

svtk_test_cxx_executable(svtkIOParallelXMLCxxTests tests)
