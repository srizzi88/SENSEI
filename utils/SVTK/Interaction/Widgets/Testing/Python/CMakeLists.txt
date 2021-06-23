if(SVTK_CAN_DO_ONSCREEN AND NOT SVTK_DEFAULT_RENDER_WINDOW_OFFSCREEN)
  svtk_add_test_python(
    TestImagePlaneWidget.py
    TestBoxWidget.py,NO_RT
    TestInteractivePlaneCutter.py
    TestInteractorEventRecorder.py,NO_RT
    TestSphereWidget.py,NO_RT
    TestTextWidget.py,NO_RT
    )
endif()
