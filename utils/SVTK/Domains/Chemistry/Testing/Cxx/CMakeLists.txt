option(SVTK_BUILD_BODR_DATA_GENERATOR
  "If enabled, the test used to generate the svtkBlueObeliskDataInternal.h header will be built."
  OFF
)
mark_as_advanced(SVTK_BUILD_BODR_DATA_GENERATOR)

set(generator_test)
if(SVTK_BUILD_BODR_DATA_GENERATOR)
  set(generator_test
    "GenerateBlueObeliskHeader.cxx,NO_VALID,NO_DATA,NO_OUTPUT"
  )
endif()

set(TestVASPAnimationReader_ARGS DATA{../Data/VASP/NPT_Z_ANIMATE.out})
set(TestVASPTessellationReader_ARGS DATA{../Data/VASP/NPT_Z_TESSELLATE.out})

svtk_add_test_cxx(svtkDomainsChemistryCxxTests tests
  ${generator_test}
  TestBallAndStick.cxx
  TestPDBBallAndStick.cxx
  TestBondColorModeDiscreteByAtom.cxx
  TestBondColorModeSingleColor.cxx
  TestCompositeRender.cxx
  TestCustomArrayRadius.cxx
  TestFastRender.cxx
  TestLiquoriceSticks.cxx
  TestMolecule.cxx,NO_VALID
  TestMoleculeIOLegacy.cxx
  TestMoleculeSelection.cxx,NO_VALID
  TestMoleculeMapperColors.cxx
  TestMoleculeMapperPropertyUpdate.cxx
  TestMoleculeToLines.cxx,NO_VALID
  TestMultiCylinderOn.cxx
  TestMultiCylinderOff.cxx
  TestPeriodicTable.cxx,NO_VALID
  TestPointSetToMoleculeFilter.cxx,NO_VALID
  TestProgrammableElectronicData.cxx,NO_VALID
  TestProteinRibbon.cxx
  TestSimpleBondPerceiver.cxx,NO_VALID
  TestVASPAnimationReader.cxx
  TestVASPTessellationReader.cxx
  TestVDWSpheres.cxx
  TestCMLMoleculeReader.cxx
  )
svtk_test_cxx_executable(svtkDomainsChemistryCxxTests tests
  RENDERING_FACTORY
  )

# On some machines this test takes 260ish seconds to complete, this allows for
# some extra time
if(NOT SVTK_TEST_TIMEOUT_TestPDBBallAndStick)
  set(SVTK_TEST_TIMEOUT_TestPDBBallAndStick 360)
endif()
set_tests_properties(SVTK::DomainsChemistryCxx-TestPDBBallAndStick
    PROPERTIES TIMEOUT ${SVTK_TEST_TIMEOUT_TestPDBBallAndStick}
               RUN_SERIAL ON)
