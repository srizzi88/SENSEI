svtk_add_test_cxx(svtkFiltersGenericCxxTests tests
  TestGenericStreamTracer.cxx
  TestGenericClip.cxx
  TestGenericContourFilter.cxx
  TestGenericCutter.cxx
  TestGenericDataSetTessellator.cxx
  TestGenericGeometryFilter.cxx
  TestGenericGlyph3DFilter.cxx
  TestGenericProbeFilter.cxx
  otherCreation.cxx,NO_VALID
  UnitTestGenericGeometryFilter.cxx,NO_VALID,NO_DATA
  )

svtk_test_cxx_executable(svtkFiltersGenericCxxTests tests)
