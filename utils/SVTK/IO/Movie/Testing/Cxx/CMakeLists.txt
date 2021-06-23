set(TEST_SRC)

if(WIN32 AND SVTK_USE_VIDEO_FOR_WINDOWS)
  list(APPEND TEST_SRC TestAVIWriter.cxx)
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_tolower)
if(svtkIOMovie_svtkoggtheora AND NOT (WIN32 AND cmake_build_type_tolower STREQUAL "debug"))
  list(APPEND TEST_SRC TestOggTheoraWriter.cxx)
endif()

svtk_add_test_cxx(svtkIOMovieCxxTests tests
  NO_VALID
  ${TEST_SRC}
  )

svtk_test_cxx_executable(svtkIOMovieCxxTests tests)
