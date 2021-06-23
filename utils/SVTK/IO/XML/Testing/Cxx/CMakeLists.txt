set(TestXML_ARGS "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/sample.xml}")
set(all_tests
  TestAMRXMLIO.cxx,NO_VALID
  TestDataObjectXMLIO.cxx,NO_VALID
  TestMultiBlockXMLIOWithPartialArrays.cxx,NO_VALID
  TestMultiBlockXMLIOWithPartialArraysTable.cxx,NO_VALID
  TestReadDuplicateDataArrayNames.cxx,NO_DATA,NO_VALID
  TestXML.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestXMLGhostCellsImport.cxx
  TestXMLHierarchicalBoxDataFileConverter.cxx,NO_VALID
  TestXMLHyperTreeGridIO.cxx,NO_VALID
  TestXMLMappedUnstructuredGridIO.cxx,NO_DATA,NO_VALID
  TestXMLToString.cxx,NO_DATA,NO_VALID,NO_OUTPUT
  TestXMLUnstructuredGridReader.cxx
  TestXMLWriterWithDataArrayFallback.cxx,NO_VALID
  )

if ((NOT DEFINED MSVC_VERSION) OR (MSVC_VERSION GREATER 1800))
  # skip TestXMLWriteRead test on MSVC 2013 and older.
  list(APPEND all_tests
    TestXMLWriteRead.cxx,NO_VALID,NO_DATA)
endif()

svtk_add_test_cxx(svtkIOXMLCxxTests tests
  ${all_tests})

# Each of these most be added in a separate svtk_add_test_cxx
svtk_add_test_cxx(svtkIOXMLCxxTests tests
  TestXMLCompositeDataReaderDistribution.cxx,NO_VALID,NO_OUTPUT
  "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/distTest.vtm,REGEX:distTest_[0-9]_0.vtp}"
  )
svtk_add_test_cxx(svtkIOXMLCxxTests tests
  TestXMLReaderBadImageData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/badImageData.xml}"
)
svtk_add_test_cxx(svtkIOXMLCxxTests tests
  TestXMLReaderBadPolyData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/badPolyData.xml}"
)
svtk_add_test_cxx(svtkIOXMLCxxTests tests
  TestXMLReaderBadRectilinearGridData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/badRectilinearGridData.xml}"
)
svtk_add_test_cxx(svtkIOXMLCxxTests tests
  TestXMLReaderBadUnstucturedGridData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/badUnstructuredGridData.xml}"
)
svtk_add_test_cxx(svtkIOXMLCxxTests tests
  TestXMLReaderBadUniformGridData,TestXMLReaderBadData.cxx,NO_VALID,NO_OUTPUT "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/badUniformGridData.xml}"
)

svtk_test_cxx_executable(svtkIOXMLCxxTests tests)

add_executable(TestXMLCInterface MACOSX_BUNDLE TestXMLCInterface.c)
target_link_libraries(TestXMLCInterface PRIVATE SVTK::IOXML)
add_test(NAME SVTK::IOXML-TestXMLCInterface COMMAND TestXMLCInterface)
