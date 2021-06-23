svtk_add_test_cxx(svtkIOImageCxxTests data_tests
  TestNrrdReader.cxx
  TestNIFTIReaderWriter.cxx
  TestNIFTIReaderAnalyze.cxx
  TestNIFTI2.cxx
  TestDICOMImageReaderFileCollection.cxx,NO_OUTPUT
  )
set(TestMetaIO_ARGS "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/HeadMRVolume.mhd,HeadMRVolume.raw}")
svtk_add_test_cxx(svtkIOImageCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestDataObjectIO.cxx
  TestMetaIO.cxx
  TestImportExport.cxx
  )

# Each of these must be added in a separate svtk_add_test_cxx
svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestJPEGReader.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/beach.jpg}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestDICOMImageReader.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/dicom/prostate.IMG}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestBMPReader.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/masonry.bmp}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestBMPReaderDoNotAllow8BitBMP.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/masonry.bmp}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestHDRReaderRGBE,TestHDRReader.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/spiaggia_di_mondello_1k.hdr}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestHDRReaderXYZ,TestHDRReader.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/gradient_xyz.hdr}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestSEPReader.cxx,NO_OUTPUT)

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestTIFFReaderMultipleMulti,TestTIFFReaderMultiple.cxx,NO_VALID,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/multipage_tiff_example.tif}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestTIFFReaderMultipleNormal,TestTIFFReaderMultiple.cxx,NO_VALID,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/test.tif}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestTIFFReaderMultipleTiled,TestTIFFReaderMultiple.cxx,NO_VALID,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/tiled_10x30_tiff_example.tif}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestTIFFReaderMulti,TestTIFFReader.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/multipage_tiff_example.tif}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestTIFFReaderTiled,TestTIFFReader.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/tiled_64x64_tiff_example.tif}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestTIFFReaderTiledRGB,TestTIFFReader.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/libtiff/gourds_tiled_200x300.tif}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestCompressedTIFFReader,TestCompressedTIFFReader.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/al_foam_smallest.0.tif}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestWriteToMemoryPNG,TestWriteToMemory.cxx,NO_DATA NO_VALID NO_OUTPUT
    "test.png")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestWriteToMemoryJPEG,TestWriteToMemory.cxx,NO_DATA NO_VALID NO_OUTPUT
    "test.jpeg")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestWriteToMemoryBMP,TestWriteToMemory.cxx,NO_DATA NO_VALID NO_OUTPUT
    "test.bmp")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestPNGReader.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/svtk.png}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestPNGReaderReadFromMemory.cxx,NO_OUTPUT
    "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/svtk.png}")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestWriteToUnicodeFileBMP,TestWriteToUnicodeFile.cxx,NO_VALID
    "image.bmp")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestWriteToUnicodeFilePNG,TestWriteToUnicodeFile.cxx,NO_VALID
    "image.png")

svtk_add_test_cxx(svtkIOImageCxxTests tests
  TestWriteToUnicodeFileJPEG,TestWriteToUnicodeFile.cxx,NO_VALID
    "image.jpg")

if (SVTK_USE_LARGE_DATA)
  svtk_add_test_cxx(svtkIOImageCxxTests large_data_tests
    TestMRCReader,TestMRCReader.cxx,NO_OUTPUT
      "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/mrc/emd_1056.mrc}")

  svtk_add_test_cxx(svtkIOImageCxxTests large_data_tests
    TestOMETIFFReaderTwoChannel,TestOMETIFFReader.cxx,NO_OUTPUT,NO_VALID
    "--data" "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/ometiff/test_small_TwoChannel_Singleframe.ome.tif}"
    "--size" "256x256x75"
    "--physical-size" "0.208x0.208x0.604"
    "--size_c" "2"
    "--size_t" "1")

  svtk_add_test_cxx(svtkIOImageCxxTests large_data_tests
    TestOMETIFFReaderSingleChannel,TestOMETIFFReader.cxx,NO_OUTPUT,NO_VALID
    "--data" "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/ometiff/test_small_singleChannel_timeSeries.ome.tif}"
    "--size" "256x256x75"
    "--physical-size" "0.208x0.208x0.604"
    "--size_c" "1"
    "--size_t" "6")

  list(APPEND data_tests ${large_data_tests})
endif()

set(all_tests
  ${data_tests}
  ${tests}
  )
svtk_test_cxx_executable(svtkIOImageCxxTests all_tests
  RENDERING_FACTORY
)
