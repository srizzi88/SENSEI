svtk_add_test_cxx(svtkFiltersProgrammableCxxTests tests
  TestProgrammableFilter.cxx,NO_VALID
  TestProgrammableGlyph.cxx
  )
svtk_test_cxx_executable(svtkFiltersProgrammableCxxTests tests
  RENDERING_FACTORY)
