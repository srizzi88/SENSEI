# TODO: Simplify this with some argument passing.
set(SVTK_PDAL_READER_TESTS)
function(add_svtk_pdal_reader_test test_input elevation)
  get_filename_component(SVTK_PDAL_READER_POSTFIX ${test_input} NAME_WE)
  string(REPLACE "-" "_" SVTK_PDAL_READER_POSTFIX ${SVTK_PDAL_READER_POSTFIX})
  set(SVTK_PDAL_READER_TEST_INPUT "${test_input}")
  set(SVTK_PDAL_READER_ELEVATION "${elevation}")
  set(test_source "TestPDALReader_${SVTK_PDAL_READER_POSTFIX}.cxx")
  configure_file(TestPDALReader.cxx.in ${test_source})
  set(SVTK_PDAL_READER_TESTS ${SVTK_PDAL_READER_TESTS} ${test_source} PARENT_SCOPE)
endfunction()

add_svtk_pdal_reader_test(test_1.las 0)
add_svtk_pdal_reader_test(test_2.las -90)

svtk_add_test_cxx(svtkIOPDALCxxTests tests
  ${SVTK_PDAL_READER_TESTS}
  )
svtk_test_cxx_executable(svtkIOPDALCxxTests tests)
