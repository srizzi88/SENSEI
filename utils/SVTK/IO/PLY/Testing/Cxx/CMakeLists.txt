svtk_add_test_cxx(svtkIOPLYCxxTests tests
  TestPLYReader.cxx
  TestPLYReaderIntensity.cxx
  TestPLYReaderPointCloud.cxx
  TestPLYWriterAlpha.cxx
  TestPLYWriter.cxx,NO_VALID
  TestPLYWriterString.cxx,NO_VALID,NO_OUTPUT
  )
svtk_add_test_cxx(svtkIOPLYCxxTests tests
  TestPLYReaderTextureUVPoints,TestPLYReaderTextureUV.cxx squareTextured.ply
  svtk.png
)
svtk_add_test_cxx(svtkIOPLYCxxTests tests
  TestPLYReaderTextureUVFaces,TestPLYReaderTextureUV.cxx squareTexturedFaces.ply
  two_svtk_logos_stacked.png
)
svtk_test_cxx_executable(svtkIOPLYCxxTests tests)
