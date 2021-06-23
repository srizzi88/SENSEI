svtk_add_test_cxx(svtkCommonExecutionModelCxxTests tests
  NO_DATA NO_VALID
  TestCopyAttributeData.cxx
  TestImageDataToStructuredGrid.cxx
  TestMetaData.cxx
  TestSetInputDataObject.cxx
  TestTemporalSupport.cxx
  TestThreadedImageAlgorithmSplitExtent.cxx
  TestTrivialConsumer.cxx
  UnitTestSimpleScalarTree.cxx
  )

svtk_add_test_cxx(svtkCommonExecutionModelCxxTests tests
  NO_VALID
  TestMultiOutputSimpleFilter.cxx
  )

svtk_test_cxx_executable(svtkCommonExecutionModelCxxTests tests)
