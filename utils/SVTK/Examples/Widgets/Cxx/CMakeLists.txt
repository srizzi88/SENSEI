cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)

project (Widgets)
find_package(SVTK REQUIRED)
svtk_module_load(
  svtkCommonCore
  svtkFiltersSources
  svtkInteractionWidgets
  svtkRenderingOpenGL2
  svtkTestingRendering
)
include(${SVTK_USE_FILE})


add_executable(Slider MACOSX_BUNDLE Slider.cxx)
target_link_libraries(Slider ${SVTK_LIBRARIES})

add_executable(Slider2D MACOSX_BUNDLE Slider2D.cxx)
target_link_libraries(Slider2D ${SVTK_LIBRARIES})

add_executable(BalloonWidget MACOSX_BUNDLE BalloonWidget.cxx)
target_link_libraries(BalloonWidget ${SVTK_LIBRARIES})

if(BUILD_TESTING)
  if(svtkTestingRendering_LOADED)
    ######## Regression Testing ########
    set(svtk-example WidgetsExamples)
    svtk_add_test_cxx(${svtk-example}CxxTests tests
      TestSlider.cxx
      TestSlider2D.cxx
      TestBalloonWidget.cxx
      )
    svtk_test_cxx_executable(${svtk-example}CxxTests tests
      RENDERING_FACTORY
      )
  endif()
endif()
