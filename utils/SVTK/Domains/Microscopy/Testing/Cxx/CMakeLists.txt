
# OpenSlideReader Tests

svtk_add_test_cxx(svtkDomainsMicroscopyCxxTests tests
  TestOpenSlideReader.cxx
  "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Microscopy/small2.ndpi}"
  )

svtk_add_test_cxx(svtkDomainsMicroscopyCxxTests  tests
    TestOpenSlideReaderPartialTIF,TestOpenSlideReaderPartial.cxx
  "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/Microscopy/pathology.tif}"
  )

svtk_test_cxx_executable(svtkDomainsMicroscopyCxxTests tests RENDERING_FACTORY)
