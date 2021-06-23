set(TestSQLiteTableReadWrite_ARGS DATA{../Data/Input/simple_table.svtk})
svtk_add_test_cxx(svtkIOSQLCxxTests tests
  NO_VALID
  TestSQLDatabaseSchema.cxx
  TestSQLiteDatabase.cxx
  TestSQLiteTableReadWrite.cxx,NO_DATA,NO_OUTPUT
  )
svtk_test_cxx_executable(svtkIOSQLCxxTests tests)

# Run these serial to avoid intermittent test failures on machines
# that do parallel testing
foreach(test IN LISTS tests)
  set_tests_properties(SVTK::IOSQLCxx-${test}
    PROPERTIES
      RUN_SERIAL 1)
endforeach()
