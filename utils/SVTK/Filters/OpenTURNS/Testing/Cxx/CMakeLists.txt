svtk_add_test_cxx(svtkFiltersOpenTURNSCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestOTDensityMap.cxx
  TestOTKernelSmoothing.cxx
  )

svtk_add_test_cxx(svtkFiltersOpenTURNSCxxTests tests
  TestScatterPlotMatrixVehiclesDensity.cxx)

svtk_test_cxx_executable(svtkFiltersOpenTURNSCxxTests tests)
