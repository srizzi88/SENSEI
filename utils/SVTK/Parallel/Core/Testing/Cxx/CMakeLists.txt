svtk_add_test_cxx(svtkParallelCoreCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestFieldDataSerialization.cxx
  TestThreadedTaskQueue.cxx
  )
svtk_test_cxx_executable(svtkParallelCoreCxxTests tests)

if (PYTHON_EXECUTABLE)
  svtk_module_test_executable(
    svtkParallelCore-TestSocketCommunicator TestSocketCommunicator.cxx)

  add_test(NAME svtkParallelCore-TestSocketCommunicator
           COMMAND ${PYTHON_EXECUTABLE}
                   ${SVTK_SOURCE_DIR}/CMake/svtkTestDriver.py
                   # indicate that the processes are not run sequentially,
                   --process
                     $<TARGET_FILE:svtkParallelCore-TestSocketCommunicator>
                     -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
                     --server
                   --process
                     $<TARGET_FILE:svtkParallelCore-TestSocketCommunicator>
                     -T ${_svtk_build_TEST_OUTPUT_DIRECTORY}
          )
endif()
