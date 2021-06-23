set(extra_tests)
if ((NOT DEFINED MSVC_VERSION) OR (MSVC_VERSION GREATER 1800))
  # skip this test on MSVC 2013 or older.
  set(extra_tests
    TestNumberToString.cxx)
endif()

svtk_add_test_cxx(svtkIOCoreCxxTests tests
  NO_VALID
  TestArrayDataWriter.cxx
  TestArrayDenormalized.cxx
  TestArraySerialization.cxx
  TestCompressLZ4.cxx
  TestCompressZLib.cxx
  TestCompressLZMA.cxx
  ${extra_tests}
  )
svtk_test_cxx_executable(svtkIOCoreCxxTests tests)
