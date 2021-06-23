svtk_add_test_cxx(svtkGUISupportQtSQLCxxTests tests
  NO_VALID
  TestQtSQLDatabase.cxx
  )

find_package(Qt5 COMPONENTS Widgets REQUIRED)

svtk_test_cxx_executable(svtkGUISupportQtSQLCxxTests tests)
target_link_libraries(svtkGUISupportQtSQLCxxTests
  PRIVATE
    Qt5::Widgets)
