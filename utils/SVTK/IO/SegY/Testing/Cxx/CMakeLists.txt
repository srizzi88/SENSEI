svtk_add_test_cxx(svtkIOSegYCxxTests tests
  TestSegY2DReader.cxx
  TestSegY2DReaderZoom.cxx
  TestSegY3DReader.cxx
  )
svtk_test_cxx_executable(svtkIOSegYCxxTests tests)
