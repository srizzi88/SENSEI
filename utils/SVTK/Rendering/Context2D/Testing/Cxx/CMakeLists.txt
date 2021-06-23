# Generic tests
svtk_add_test_cxx(svtkRenderingContext2DCxxTests tests
  TestContext2D.cxx,NO_VALID
  TestPolyDataToContext.cxx
  )

svtk_test_cxx_executable(svtkRenderingContext2DCxxTests tests)
