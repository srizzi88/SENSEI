svtk_add_test_cxx(svtkImagingCoreCxxTests tests
  FastSplatter.cxx
  ImageAccumulate.cxx,NO_VALID
  ImageAccumulateLarge.cxx,NO_VALID,NO_DATA,NO_OUTPUT 32
  ImageAutoRange.cxx
  ImageBSplineCoefficients.cxx
  ImageHistogram.cxx
  ImageHistogramStatistics.cxx,NO_VALID
  ImageInterpolateSlidingWindow2D.cxx
  ImageInterpolateSlidingWindow3D.cxx
  ImageResize.cxx
  ImageResize3D.cxx
  ImageResizeCropping.cxx
  ImageReslice.cxx
  ImageWeightedSum.cxx,NO_VALID
  ImportExport.cxx,NO_VALID
  TestBSplineWarp.cxx
  TestImageStencilDataMethods.cxx,NO_VALID
  TestImageStencilIterator.cxx,NO_VALID
  TestStencilWithLasso.cxx
  TestStencilWithPolyDataContour.cxx
  TestStencilWithPolyDataSurface.cxx
  TestUpdateExtentReset.cxx,NO_VALID
  )
list(APPEND tests
  TestImageStencilData.cxx
  )

# The stencil test is special
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::ImagingCoreCxx-AddStencilData
  COMMAND svtkImagingCoreCxxTests TestImageStencilData 1
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V DATA{../Data/Baseline/TestAddStencilData.png,:})
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::ImagingCoreCxx-SubtractStencilData
  COMMAND svtkImagingCoreCxxTests TestImageStencilData 2
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V DATA{../Data/Baseline/TestSubtractStencilData.png,:})
ExternalData_add_test(${_svtk_build_TEST_DATA_TARGET}
  NAME SVTK::ImagingCoreCxx-ClipStencilData
  COMMAND svtkImagingCoreCxxTests TestImageStencilData 3
    -D ${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}
    -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
    -V DATA{../Data/Baseline/TestClipStencilData.png,:})

svtk_test_cxx_executable(svtkImagingCoreCxxTests tests
  RENDERING_FACTORY
  )
