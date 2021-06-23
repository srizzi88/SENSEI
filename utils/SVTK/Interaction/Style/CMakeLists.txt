set(classes
  svtkInteractorStyleDrawPolygon
  svtkInteractorStyleFlight
  svtkInteractorStyleImage
  svtkInteractorStyleJoystickActor
  svtkInteractorStyleJoystickCamera
  svtkInteractorStyleMultiTouchCamera
  svtkInteractorStyleRubberBand2D
  svtkInteractorStyleRubberBand3D
  svtkInteractorStyleRubberBandPick
  svtkInteractorStyleRubberBandZoom
  svtkInteractorStyleTerrain
  svtkInteractorStyleTrackball
  svtkInteractorStyleTrackballActor
  svtkInteractorStyleTrackballCamera
  svtkInteractorStyleUnicam
  svtkInteractorStyleUser

  svtkInteractorStyleSwitch
  svtkParallelCoordinatesInteractorStyle)

if (ANDROID OR APPLE_IOS)
  set(style_override "svtkInteractorStyleMultiTouchCamera")
else ()
  set(style_override "svtkInteractorStyleSwitch")
endif ()
svtk_object_factory_declare(
  BASE svtkInteractorStyleSwitchBase
  OVERRIDE "${style_override}")

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKINTERACTIONSTYLE_EXPORT")

svtk_module_add_module(SVTK::InteractionStyle
  CLASSES ${classes}
  SOURCES ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header})
