svtk_add_test_cxx(svtkGUISupportQtCxxTests tests
  NO_VALID
  TestQtDebugLeaksView.cxx
  TestQtTableModelAdapter.cxx
  TestQtTreeModelAdapter.cxx
  )

svtk_add_test_cxx(svtkGUISupportQtCxxTests tests
  -w QSVTKOpenGLStereoWidget
  TestQSVTKOpenGLStereoWidgetQWidgetWidget,TestQWidgetWidget.cxx,NO_VALID
  TestQSVTKOpenGLStereoWidgetPicking,TestQtPicking.cxx,NO_VALID
  TestQSVTKOpenGLStereoWidgetSwapWindows,TestQtSwapWindows.cxx,NO_VALID
  TestQSVTKOpenGLStereoWidget,TestQtWidget.cxx
  TestQSVTKOpenGLStereoWidgetWithDisabledInteractor,TestQtWithDisabledInteractor.cxx
  TestQSVTKOpenGLStereoWidgetWithMSAA,TestQtWithMSAA.cxx
  )

svtk_add_test_cxx(svtkGUISupportQtCxxTests tests
  -w QSVTKOpenGLNativeWidget
  TestQSVTKOpenGLNativeWidgetQWidgetWidget,TestQWidgetWidget.cxx,NO_VALID
  TestQSVTKOpenGLNativeWidgetPicking,TestQtPicking.cxx,NO_VALID
  TestQSVTKOpenGLNativeWidgetSwapWindows,TestQtSwapWindows.cxx,NO_VALID
  TestQSVTKOpenGLNativeWidget,TestQtWidget.cxx
  TestQSVTKOpenGLNativeWidgetWithDisabledInteractor,TestQtWithDisabledInteractor.cxx
  TestQSVTKOpenGLNativeWidgetWithMSAA,TestQtWithMSAA.cxx
  )

svtk_add_test_cxx(svtkGUISupportQtCxxTests tests
  -w QSVTKOpenGLWindow
  TestQSVTKOpenGLWindowQWidgetWidget,TestQWidgetWidget.cxx,NO_VALID
  TestQSVTKOpenGLWindowPicking,TestQtPicking.cxx,NO_VALID
  TestQSVTKOpenGLWindow,TestQtWidget.cxx
  TestQSVTKOpenGLWindowWithDisabledInteractor,TestQtWithDisabledInteractor.cxx
  TestQSVTKOpenGLWindowWithMSAA,TestQtWithMSAA.cxx
  )

svtk_add_test_cxx(svtkGUISupportQtCxxTests tests
  -w QSVTKRenderWidget
  TestQSVTKRenderWidgetQWidgetWidget,TestQWidgetWidget.cxx,NO_VALID
  TestQSVTKRenderWidgetPicking,TestQtPicking.cxx,NO_VALID
  TestQSVTKRenderWidgetSwapWindows,TestQtSwapWindows.cxx,NO_VALID
  TestQSVTKRenderWidget,TestQtWidget.cxx
  TestQSVTKRenderWidgetWithDisabledInteractor,TestQtWithDisabledInteractor.cxx
  TestQSVTKRenderWidgetWithMSAA,TestQtWithMSAA.cxx
  )

if (NOT SVTK_LEGACY_REMOVE)
svtk_add_test_cxx(svtkGUISupportQtCxxTests tests
  -w QSVTKOpenGLWidget
  TestQSVTKOpenGLWidgetQWidgetWidget,TestQWidgetWidget.cxx,NO_VALID
  TestQSVTKOpenGLWidgetPicking,TestQtPicking.cxx,NO_VALID
  TestQSVTKOpenGLWidgetSwapWindows,TestQtSwapWindows.cxx,NO_VALID
  TestQSVTKOpenGLWidget,TestQtWidget.cxx
  TestQSVTKOpenGLWidgetWithDisabledInteractor,TestQtWithDisabledInteractor.cxx
  TestQSVTKOpenGLWidgetWithMSAA,TestQtWithMSAA.cxx
  )
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)

svtk_test_cxx_executable(svtkGUISupportQtCxxTests tests
  QTestApp.cxx
  TestQtCommon.cxx)

target_link_libraries(svtkGUISupportQtCxxTests
  PRIVATE
    Qt5::Widgets)

set_tests_properties(SVTK::GUISupportQtCxx-TestQSVTKOpenGLStereoWidgetPicking
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(SVTK::GUISupportQtCxx-TestQSVTKOpenGLNativeWidgetPicking
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(SVTK::GUISupportQtCxx-TestQSVTKOpenGLWindowPicking
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(SVTK::GUISupportQtCxx-TestQSVTKRenderWidgetPicking
 PROPERTIES RUN_SERIAL ON)

if (NOT SVTK_LEGACY_REMOVE)
  set_tests_properties(SVTK::GUISupportQtCxx-TestQSVTKOpenGLWidgetPicking
   PROPERTIES RUN_SERIAL ON)
endif()


# since these tests grab from on-screen buffer, they must be run serially.
set_tests_properties(SVTK::GUISupportQtCxx-TestQSVTKOpenGLStereoWidgetWithDisabledInteractor
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(SVTK::GUISupportQtCxx-TestQSVTKOpenGLNativeWidgetWithDisabledInteractor
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(SVTK::GUISupportQtCxx-TestQSVTKOpenGLWindowWithDisabledInteractor
 PROPERTIES RUN_SERIAL ON)
set_tests_properties(SVTK::GUISupportQtCxx-TestQSVTKRenderWidgetWithDisabledInteractor
 PROPERTIES RUN_SERIAL ON)

if (NOT SVTK_LEGACY_REMOVE)
  set_tests_properties(SVTK::GUISupportQtCxx-TestQSVTKOpenGLWidgetWithDisabledInteractor
   PROPERTIES RUN_SERIAL ON)
endif()
