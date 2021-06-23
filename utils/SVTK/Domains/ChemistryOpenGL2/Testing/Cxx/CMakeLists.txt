svtk_add_test_cxx(svtkDomainsChemistryOpenGL2CxxTests tests
  TestPDBBallAndStickShadows.cxx
  TestPDBBallAndStickShadowsDOFSSAA.cxx
  TestPDBBallAndStickTranslucent.cxx
  )
svtk_test_cxx_executable(svtkDomainsChemistryOpenGL2CxxTests tests
  RENDERING_FACTORY
  )

# On some machines this test takes 260ish seconds to complete, this allows for
# some extra time
if(NOT SVTK_TEST_TIMEOUT_TestPDBBallAndStickShadows)
  set(SVTK_TEST_TIMEOUT_TestPDBBallAndStickShadows 360)
endif()
set_tests_properties(SVTK::DomainsChemistryOpenGL2Cxx-TestPDBBallAndStickShadows
    PROPERTIES TIMEOUT ${SVTK_TEST_TIMEOUT_TestPDBBallAndStickShadows}
               RUN_SERIAL ON)
