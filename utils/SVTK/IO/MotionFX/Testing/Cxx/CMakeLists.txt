svtk_add_test_cxx(svtkIOMotionFXCxxTests tests
  NO_OUTPUT
  TestMotionFXCFGReader2Gears.cxx
  TestMotionFXCFGReaderPlanetary.cxx
  TestMotionFXCFGReaderPositionFile.cxx
  )
svtk_test_cxx_executable(svtkIOMotionFXCxxTests tests)
