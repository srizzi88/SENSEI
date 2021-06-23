if(SVTK_MYSQL_TEST_URL)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/svtkIOMySQLTestingCxxConfigure.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/svtkIOMySQLTestingCxxConfigure.h)

  include_directories(${CMAKE_CURRENT_BINARY_DIR})

  if(WIN32)
    set(TestMySQLTableReadWrite_ARGS
      DATA{../Data/Input/simple_table_win.svtk})
  else()
    set(TestMySQLTableReadWrite_ARGS
      DATA{../Data/Input/simple_table.svtk})
  endif()
  svtk_add_test_cxx(svtkIOMySQLCxxTests tests
    NO_DATA NO_VALID
    TestMySQLDatabase.cxx
    TestMySQLTableReadWrite.cxx
    )

  svtk_test_cxx_executable(svtkIOMySQLCxxTests tests)
endif()
