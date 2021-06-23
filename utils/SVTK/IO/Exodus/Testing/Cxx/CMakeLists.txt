
# We need typed arrays to be supported by the dispatcher to run the insitu test:
if(SVTK_DISPATCH_TYPED_ARRAYS)
  set(extra_tests ${extra_tests}
    TestInSituExodus.cxx,NO_VALID
  )
endif()

svtk_add_test_cxx(svtkIOExodusCxxTests tests
  TestExodusAttributes.cxx,NO_VALID,NO_OUTPUT
  TestExodusIgnoreFileTime.cxx,NO_VALID,NO_OUTPUT
  TestExodusSideSets.cxx,NO_VALID,NO_OUTPUT
  TestMultiBlockExodusWrite.cxx
  ${extra_tests}
  )

list(APPEND tests
  TestExodusTetra15.cxx
  TestExodusWedge21.cxx
)

svtk_test_cxx_executable(svtkIOExodusCxxTests tests
  RENDERING_FACTORY)

ExternalData_add_test(SVTKData
  NAME SVTK::IOExodusCxx-Tetra15
  COMMAND svtkIOExodusCxxTests TestExodusTetra15
    -D ${SVTK_TEST_DATA_DIR}
    -V DATA{../Data/Baseline/TestExodusTetra15.png}
    -T "${SVTK_TEST_OUTPUT_DIR}"
    DATA{../../../../Testing/Data/tetra15.g}
)

ExternalData_add_test(SVTKData
  NAME SVTK::IOExodusCxx-Wedge21
  COMMAND svtkIOExodusCxxTests TestExodusWedge21
    -D ${SVTK_TEST_DATA_DIR}
    -V DATA{../Data/Baseline/TestExodusWedge21.png}
    -T "${SVTK_TEST_OUTPUT_DIR}"
    DATA{../../../../Testing/Data/wedge21.g}
)
