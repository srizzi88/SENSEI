cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)


project (VisualizationAlgorithms)
find_package(SVTK REQUIRED)
svtk_module_config(SVTK
  svtkCommonCore
  svtkCommonDataModel
  svtkFiltersCore
  svtkFiltersModeling
  svtkIOXML
  svtkInteractionStyle
  svtkRenderingOpenGL2
  svtkTestingRendering
)
include(${SVTK_USE_FILE})

add_executable(TubesWithVaryingRadiusAndColors MACOSX_BUNDLE TubesWithVaryingRadiusAndColors.cxx)
add_executable(FilledContours MACOSX_BUNDLE FilledContours.cxx)
add_executable(BandedContours MACOSX_BUNDLE BandedContours.cxx)

target_link_libraries(TubesWithVaryingRadiusAndColors ${SVTK_LIBRARIES})
target_link_libraries(FilledContours ${SVTK_LIBRARIES})
target_link_libraries(BandedContours ${SVTK_LIBRARIES})

if(BUILD_TESTING)
  if(svtkTestingRendering_LOADED)
    ######## Regression Testing ########
    set(svtk-example VisualizationAlgorithmsExamples)
    set(TestFilledContours_ARGS "DATA{${SVTK_TEST_INPUT_DIR}/filledContours.vtp}" 10)
    set(TestBandedContours_ARGS "DATA{${SVTK_TEST_INPUT_DIR}/filledContours.vtp}" 10)
    svtk_add_test_cxx(${svtk-example}CxxTests tests
      TestTubesWithVaryingRadiusAndColors.cxx
      TestFilledContours.cxx
      TestBandedContours.cxx
      )
    svtk_test_cxx_executable(${svtk-example}CxxTests tests
      RENDERING_FACTORY
      )
  endif()
endif()
