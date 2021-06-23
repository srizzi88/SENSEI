set(TestOrientedGlyphContour_ARGS "--PlaybackFile" "DATA{../Data/Input/TestOrientedGlyphContourEventLog.txt}")
set(TerrainPolylineEditor_ARGS -ProjectionMode 2 -HeightOffset 100)
svtk_add_test_cxx(svtkInteractionWidgetsCxxTests tests
  BoxWidget.cxx
  BoxWidget2.cxx
  ImagePlaneWidget.cxx
  TerrainPolylineEditor.cxx
  TestAffineWidget.cxx
  TestAngleWidget2D.cxx
  TestAngleWidget3D.cxx
  TestBalloonWidget.cxx
  TestBiDimensionalWidget.cxx
  TestBorderWidget.cxx
  TestBrokenLineWidget.cxx
  TestCameraWidget.cxx
  TestCaptionWidget.cxx
  TestCellCentersPointPlacer.cxx
  TestCenteredSliderWidget2D.cxx
  TestCheckerboardWidget.cxx
  TestCoincidentHandleWidgets.cxx
  TestConstrainedHandleWidget.cxx
  TestContourWidget2.cxx
  TestDijkstraImageGeodesicPath.cxx
  TestDistanceWidget.cxx
  TestDistanceWidget3D.cxx
  TestFinitePlaneWidget.cxx
  TestFixedSizeHandleRepresentation3D.cxx
  TestFocalPlaneContour.cxx
  TestHandleWidget.cxx
  TestHandleWidget2D.cxx
  TestImageActorContourWidget.cxx
  TestImplicitCylinderWidget.cxx
  TestImplicitCylinderWidget2.cxx
  TestImplicitPlaneWidget.cxx
  TestImplicitPlaneWidget2.cxx
  TestImplicitPlaneWidget2LockNormalToCamera.cxx
  TestImplicitPlaneWidget2b.cxx
  TestImplicitPlaneWidget3.cxx
  TestLightWidget.cxx
  TestLogoWidget.cxx
  TestLogoWidgetAlphaBlending.cxx
  TestLogoWidgetDepthPeeling.cxx
  TestMeasurementCubeHandleRepresentation3D.cxx
  TestMultipleViewports.cxx
  TestOrientationMarkerWidget.cxx
  TestOrientationMarkerWidget2.cxx
  TestOrthoPlanes.cxx
  TestParallelopipedWidget.cxx
  TestPickingManagerSeedWidget.cxx
  TestPickingManagerSeedWidget2.cxx
  TestPlaybackWidget.cxx
  TestPointHandleRepresentation3D.cxx
  TestPolylineWidget.cxx
  TestProgrammaticPlacement.cxx
  TestProgressBarWidget.cxx
  TestProp3DButtonRepresentation.cxx,NO_VALID
  TestRectilinearWipeWidget.cxx
  TestResliceCursorWidget2.cxx
  TestResliceCursorWidget3.cxx
  TestScalarBarWidget.cxx
  TestSeedWidget.cxx
  TestSeedWidget2.cxx
  TestSeedWidgetNonUniformRepresentations.cxx
  TestSliderWidget.cxx
  TestSliderWidget2D.cxx
  TestSliderWidgetMultipleViewports.cxx
  TestSphereWidget2CenterCursor.cxx
  TestSphereWidgetZoomInOut.cxx
  TestSplineWidget.cxx
  TestTextWidget.cxx
  )
svtk_add_test_cxx(svtkInteractionWidgetsCxxTests no_valid_tests
  NO_VALID
  svtkAngleRepresentation2DTest1.cxx
  svtkAngleRepresentation3DTest1.cxx
  svtkAngleWidgetTest1.cxx
  svtkBalloonRepresentationTest1.cxx
  svtkBalloonWidgetTest1.cxx
  svtkBiDimensionalRepresentation2DTest1.cxx
  svtkBiDimensionalWidgetTest1.cxx
  svtkBorderWidgetTest1.cxx
  svtkCaptionRepresentationTest1.cxx
  svtkCaptionWidgetTest1.cxx
  svtkConstrainedPointHandleRepresentationTest1.cxx
  svtkHoverWidgetTest1.cxx
  svtkLineRepresentationTest1.cxx
  svtkLineWidget2Test1.cxx
  svtkPolyLineRepresentationTest1.cxx
  svtkPolyLineWidgetTest1.cxx
  svtkSeedRepresentationTest1.cxx
  svtkSeedWidgetTest1.cxx
  svtkSplineRepresentationTest1.cxx
  svtkSplineWidget2Test1.cxx
  svtkTextRepresentationTest1.cxx
  svtkTextWidgetTest1.cxx
  )

# Tests with data
svtk_add_test_cxx(svtkInteractionWidgetsCxxTests data_tests
  TestButtonWidget.cxx
  TestButtonWidgetMultipleViewports.cxx
  TestDijkstraGraphGeodesicPath.cxx
  TestLineWidget.cxx
  TestLineWidget2.cxx
  TestOrientedGlyphContour.cxx
  TestPlaneWidget.cxx
  TestPointWidget.cxx
  TestPolyPlane.cxx,NO_VALID
  TestPolygonalHandleRepresentations.cxx
  TestPolygonalRepresentationHandleWidget.cxx
  TestPolygonalSurfaceConstrainedDistanceWidget.cxx
  TestSphereHandleWidget.cxx
  TestSurfaceConstrainedHandleWidget.cxx
  TestTensorProbeWidget.cxx
  TestImageTracerWidget.cxx
  )

if(NOT SVTK_OPENGL_HAS_OSMESA)
  svtk_add_test_cxx(svtkInteractionWidgetsCxxTests ogl_tests
    TestPickingManagerWidgets.cxx
    )
endif()

set(all_tests
  ${tests}
  ${no_valid_tests}
  ${data_tests}
  ${ogl_tests}
  )
svtk_test_cxx_executable(svtkInteractionWidgetsCxxTests all_tests
  RENDERING_FACTORY
  )

# Allow local customization of timeout for this test.
if(NOT SVTK_TEST_TIMEOUT_TerrainPolylineEditor)
  set(SVTK_TEST_TIMEOUT_TerrainPolylineEditor 90)
endif()
set_tests_properties(SVTK::InteractionWidgetsCxx-TerrainPolylineEditor
  PROPERTIES TIMEOUT ${SVTK_TEST_TIMEOUT_TerrainPolylineEditor})

if(NOT SVTK_OPENGL_HAS_OSMESA)
  # Allow local customization of timeout for this test.
  if(NOT SVTK_TEST_TIMEOUT_TestPickingManagerSeedWidget)
    set(SVTK_TEST_TIMEOUT_TestPickingManagerSeedWidget 60)
  endif()
  set_tests_properties(SVTK::InteractionWidgetsCxx-TestPickingManagerSeedWidget
    PROPERTIES TIMEOUT ${SVTK_TEST_TIMEOUT_TestPickingManagerSeedWidget})
endif()

# On some machines this test takes ~220 seconds to complete, allow for some
# extra time
if(NOT SVTK_TEST_TIMEOUT_TestSphereHandleWidget)
  set(SVTK_TEST_TIMEOUT_TestSphereHandleWidget 300)
endif()
set_tests_properties(SVTK::InteractionWidgetsCxx-TestSphereHandleWidget
    PROPERTIES TIMEOUT ${SVTK_TEST_TIMEOUT_TestSphereHandleWidget}
               RUN_SERIAL ON)
