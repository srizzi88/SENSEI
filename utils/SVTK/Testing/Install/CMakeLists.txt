# Test "make install"
if (CMAKE_INSTALL_PREFIX STREQUAL "${SVTK_BINARY_DIR}/InstallTest")
  add_test(
    NAME    Install
    COMMAND "${CMAKE_COMMAND}"
            -DCONFIGURATION=$<CONFIGURATION>
            "-DSVTK_BINARY_DIR=${SVTK_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/InstallTest.cmake")
endif ()
