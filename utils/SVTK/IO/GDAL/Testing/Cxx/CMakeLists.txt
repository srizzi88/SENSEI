set(TestGDALRasterNoDataValue_ARGS
  -D DATA{../Data/Input/TestGDALRasterNoDataValue.tif}
)
set(TestGDALRasterPalette_ARGS
  -D DATA{../Data/Input/TestGDALRasterPalette.tif}
)

svtk_add_test_cxx(svtkIOGDALCxxTests tests
  TestGDALVectorReader.cxx
  TestGDALRasterReader.cxx
  TestGDALRasterNoDataValue.cxx,NO_VALID,NO_OUTPUT
  TestGDALRasterPalette.cxx
  )
svtk_test_cxx_executable(svtkIOGDALCxxTests tests)
