if(SVTK_ODBC_TEST_DSN)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/svtkIOODBCTestingCxxConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/svtkIOODBCTestingCxxConfigure.h)

  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  svtk_add_test_cxx(svtkIOODBCCxxTests tests
    NO_DATA NO_VALID
    TestODBCDatabase.cxx
    )
  svtk_test_cxx_executable(svtkIOODBCCxxTests tests)
endif()
