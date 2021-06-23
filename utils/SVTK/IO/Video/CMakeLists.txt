set(classes
  svtkVideoSource)

if (WIN32)
  include(svtkTestVideoForWindows)
  if (SVTK_VFW_SUPPORTS_CAPTURE)
    list(APPEND classes svtkWin32VideoSource)
  endif ()

  if (SVTK_USE_MATROX_IMAGING)
    # Include directory needed for svtkMILVideoSource.cxx.
    list(APPEND classes svtkMILVideoSource)
  endif ()
endif ()

svtk_module_add_module(SVTK::IOVideo
  CLASSES ${classes})

if (WIN32 AND SVTK_USE_VIDEO_FOR_WINDOWS)
  svtk_module_link(SVTK::IOVideo
    PRIVATE
      vfw32)
endif ()
