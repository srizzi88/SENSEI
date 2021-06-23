svtk_add_test_cxx(svtkIOParallelCxxTests tests
  TestPOpenFOAMReader.cxx
  TestBigEndianPlot3D.cxx,NO_VALID
  )
svtk_test_cxx_executable(svtkIOParallelCxxTests tests)
