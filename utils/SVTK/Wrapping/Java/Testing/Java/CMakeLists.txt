find_package(Java REQUIRED COMPONENTS Runtime Development)

set(java_simple_test_names
  ConcurrencyGC
  JavaDelete
  ManualGC
  JavaGCAndDelete)

set(java_test_names
  Regression
  ${java_simple_test_names})

set(java_test_files)
foreach (java_test IN LISTS java_test_names)
  list(APPEND java_test_files
    "${CMAKE_CURRENT_SOURCE_DIR}/svtk/test/${java_test}.java")
endforeach ()

set(classpath_separator ":")
if (WIN32)
  set(classpath_separator "\\;")
endif ()
set(svtk_test_classpath
  "$<TARGET_FILE:SVTK::svtkjava>")
string(REPLACE ";" "${classpath_separator}" svtk_test_classpath "${svtk_test_classpath}")

add_library(svtkjava_tests STATIC
  ${java_test_files})
target_compile_options(svtkjava_tests
  PRIVATE
    "SHELL:-classpath \"${svtk_test_classpath}\""
    "SHELL:-source ${SVTK_JAVA_SOURCE_VERSION}"
    "SHELL:-target ${SVTK_JAVA_TARGET_VERSION}")
target_link_libraries(svtkjava_tests
  PRIVATE
    SVTK::svtkjava)

set(svtk_test_classpath
  "$<TARGET_FILE:svtkjava_tests>"
  "$<TARGET_FILE:SVTK::svtkjava>")
string(REPLACE ";" "${classpath_separator}" svtk_test_classpath "${svtk_test_classpath}")

if (TARGET SVTK::RenderingOpenGL2)
  ExternalData_add_test(SVTKData
    NAME    svtkJavaTests-Regression
    COMMAND "${Java_JAVA_EXECUTABLE}"
            -classpath "${svtk_test_classpath}"
            svtk.test.Regression
            -D "${CMAKE_BINARY_DIR}/ExternalData"
            -V "DATA{../Data/Baseline/Cone.png}"
            -T "${CMAKE_BINARY_DIR}/Testing/Temporary")
endif ()

foreach (java_simple_test IN LISTS java_simple_test_names)
  add_test(
    NAME    "svtkJavaTests-${java_simple_test}"
    COMMAND "${Java_JAVA_EXECUTABLE}"
            -classpath "${svtk_test_classpath}"
            "svtk.test.${java_simple_test}"
            -T "${CMAKE_BINARY_DIR}/Testing/Temporary")
endforeach ()
