set(classes
  svtkGenericMovieWriter)

if (WIN32)
  # Check if VideoForWindows is supported. This also adds an option
  # SVTK_USE_VIDEO_FOR_WINDOWS that is set to ON/OFF by default based on
  # whether VideoForWindows was found.
  include(svtkTestVideoForWindows)

  if (SVTK_USE_VIDEO_FOR_WINDOWS)
    list(APPEND classes svtkAVIWriter)
  endif ()
endif ()

svtk_module_add_module(SVTK::IOMovie
  CLASSES ${classes})

if (WIN32 AND SVTK_USE_VIDEO_FOR_WINDOWS)
  svtk_module_link(SVTK::IOMovie
    PRIVATE
      vfw32)
endif ()
