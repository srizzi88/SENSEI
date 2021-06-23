# Tests with data
ExternalData_Expand_Arguments(SVTKData _
  "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/AMReX/MFIX-Exa/,RECURSE:,REGEX:.*}"
  "DATA{${_svtk_build_TEST_INPUT_DATA_DIRECTORY}/Data/AMReX/Sample2D/,RECURSE:,REGEX:.*}"
  )

svtk_add_test_cxx(svtkIOAMRCxxTests tests
  NO_VALID NO_OUTPUT
  TestAMReXParticlesReader.cxx
  TestEnzoReader.cxx
  )
svtk_test_cxx_executable(svtkIOAMRCxxTests tests)
