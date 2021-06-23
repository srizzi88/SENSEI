set(test_64bit)
if(${SVTK_USE_64BIT_IDS})
  set(test_64bit
    TestExtractValues.cxx,NO_VALID DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/cad_cubes.vtp}
    )
endif()

svtk_add_test_cxx(svtkFiltersExtractionCxxTests tests
  TestConvertSelection.cxx,NO_VALID
  TestExpandMarkedElements.cxx
  TestExtractBlock.cxx,NO_VALID,NO_DATA
  TestExtractDataArraysOverTime.cxx,NO_VALID
  TestExtractThresholdsMultiBlock.cxx,NO_VALID
  TestExtraction.cxx
  TestExtractionExpression.cxx
  TestExtractRectilinearGrid.cxx,NO_VALID,NO_DATA
  TestExtractRows.cxx,NO_VALID,NO_DATA
  TestExtractSelectedArraysOverTime.cxx,NO_VALID
  TestExtractSelection.cxx
  TestExtractTimeSteps.cxx,NO_VALID
  ${test_64bit}
  )
svtk_test_cxx_executable(svtkFiltersExtractionCxxTests tests)
