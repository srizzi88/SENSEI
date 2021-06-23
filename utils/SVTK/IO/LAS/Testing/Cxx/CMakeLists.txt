# TODO: Simplify this with some argument passing.
set(SVTK_LAS_READER_TESTS)
function(add_svtk_las_reader_test test_input elevation)
  get_filename_component(SVTK_LAS_READER_POSTFIX ${test_input} NAME_WE)
  set(SVTK_LAS_READER_TEST_INPUT "${test_input}")
  set(SVTK_LAS_READER_ELEVATION "${elevation}")
  set(test_source "TestLASReader_${SVTK_LAS_READER_POSTFIX}.cxx")
  configure_file(TestLASReader.cxx.in ${test_source})
  set(SVTK_LAS_READER_TESTS ${SVTK_LAS_READER_TESTS} ${test_source} PARENT_SCOPE)
endfunction()

add_svtk_las_reader_test(test_1.las 0)
add_svtk_las_reader_test(test_2.las -90)

svtk_add_test_cxx(svtkIOLASCxxTests tests
  ${SVTK_LAS_READER_TESTS}
  )
svtk_test_cxx_executable(svtkIOLASCxxTests tests)
