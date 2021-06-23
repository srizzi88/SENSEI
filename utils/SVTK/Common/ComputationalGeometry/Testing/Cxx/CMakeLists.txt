svtk_add_test_cxx(svtkCommonComputationalGeometryCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  UnitTestParametricSpline.cxx
  TestBilinearQuadIntersection.cxx
  )
svtk_test_cxx_executable(svtkCommonComputationalGeometryCxxTests tests)
