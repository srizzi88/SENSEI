if(SVTK_PSQL_TEST_URL)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/svtkIOPostgresSQLTestingCxxConfigure.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/svtkIOPostgresSQLTestingCxxConfigure.h)

  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  if(WIN32)
    set(TestPostgreSQLTableReadWrite_ARGS
      DATA{../Data/Input/simple_table_win.svtk})
  else()
    set(TestPostgreSQLTableReadWrite_ARGS
      DATA{../Data/Input/simple_table.svtk})
  endif()
  svtk_add_test_cxx(svtkIOPostgreSQLCxxTests tests
    NO_DATA NO_VALID
    TestPostgreSQLDatabase.cxx
    TestPostgreSQLTableReadWrite.cxx
    )

  svtk_test_cxx_executable(svtkIOPostgreSQLCxxTests tests)
endif()
