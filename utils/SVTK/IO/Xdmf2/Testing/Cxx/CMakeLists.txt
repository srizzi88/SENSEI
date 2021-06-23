svtk_module_test_data(
  Data/XDMF/,REGEX:.*)

svtk_add_test_cxx(svtkIOXdmf2CxxTests tests
  TestTemporalXdmfReaderWriter.cxx,NO_VALID
  XdmfTestSVTKIO.cxx,NO_VALID
  )
svtk_test_cxx_executable(svtkIOXdmf2CxxTests tests)
