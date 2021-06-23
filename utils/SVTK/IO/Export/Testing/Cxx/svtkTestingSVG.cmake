# svtk_add_svg_test(<test> [<another test> <yet another test> ...])
#
# Takes a list of test source files as arguments and adds additional tests
# to convert an SVG output into png (RasterizePNG) and validates against a
# baseline (VerifyRasterizedPNG).
#
# This function does not replace svtk_add_test_cxx, but supplements it -- this
# only creates the rasterize/verify tests, svtk_add_test_cxx is needed to create
# the test that generates the original SVG output.
function(svtk_add_svg_test)
  set(tests ${ARGN})
  foreach(test ${tests})
    string(REGEX REPLACE ",.*" "" testsrc "${test}")
    get_filename_component(TName ${testsrc} NAME_WE)

    # Convert svg to png
    add_test(NAME ${_svtk_build_test}Cxx-${TName}-RasterizePNG
      COMMAND ${CMAKE_COMMAND}
        "-DSVGFILE=${_svtk_build_TEST_OUTPUT_DIRECTORY}/${TName}.svg"
        "-DPNGFILE=${_svtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-raster.png"
        "-DCONVERTER=${SVTK_WKHTMLTOIMAGE_EXECUTABLE}"
        -DREMOVESVG=1
        -P "${SVTK_SOURCE_DIR}/CMake/RasterizeSVG.cmake"
    )
    set_tests_properties("${_svtk_build_test}Cxx-${TName}-RasterizePNG"
      PROPERTIES
        DEPENDS "${_svtk_build_test}Cxx-${TName}"
        REQUIRED_FILES "${_svtk_build_TEST_OUTPUT_DIRECTORY}/${TName}.svg"
        LABELS "${svtkIOExport_TEST_LABELS}"
    )

    get_filename_component(TName ${test} NAME_WE)
    if(${${TName}Error})
      set(_error_threshold ${${TName}Error})
    else()
      set(_error_threshold 15)
    endif()

    # Image diff rasterized png produced from SVG with baseline
    ExternalData_add_test(SVTKData
      NAME ${_svtk_build_test}Cxx-${TName}-VerifyRasterizedPNG
      COMMAND "svtkRenderingGL2PSOpenGL2CxxTests" PNGCompare
        -D "${_svtk_build_TEST_OUTPUT_DATA_DIRECTORY}"
        -T "${_svtk_build_TEST_OUTPUT_DIRECTORY}"
        -E "${_error_threshold}"
        -V "DATA{../Data/Baseline/${TName}-rasterRef.png,:}"
        --test-file "${_svtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-raster.png"
    )
    set_tests_properties("${_svtk_build_test}Cxx-${TName}-VerifyRasterizedPNG"
      PROPERTIES
        DEPENDS "${_svtk_build_test}Cxx-${TName}-RasterizePNG"
        REQUIRED_FILES "${_svtk_build_TEST_OUTPUT_DIRECTORY}/${TName}-raster.png"
        LABELS "${_svtk_build_test_labels}"
        )
  endforeach()
endfunction()

set(svtkTestingSVG_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
