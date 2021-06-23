
set(classes
  svtkGenericRenderWindowInteractor
)

# Setup factory overrides and add window system specific render windows.
if (SVTK_USE_X)
  # Add some custom overrides
  if (NOT SVTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    svtk_object_factory_declare(
      BASE svtkRenderWindowInteractor
      OVERRIDE svtkXRenderWindowInteractor)
  endif ()
  list(APPEND classes svtkXRenderWindowInteractor)
elseif (WIN32)
  # Add some custom overrides
  if (NOT SVTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    svtk_object_factory_declare(
      BASE svtkRenderWindowInteractor
      OVERRIDE svtkWin32RenderWindowInteractor)
    svtk_object_factory_declare(
      BASE svtkHardwareWindow
      OVERRIDE svtkWin32HardwareWindow)
  endif ()
  list(APPEND classes svtkWin32HardwareWindow svtkWin32RenderWindowInteractor)
elseif (SVTK_USE_COCOA)
  # Add some custom overrides
  if (NOT SVTK_DEFAULT_RENDER_WINDOW_HEADLESS)
    svtk_object_factory_declare(
      BASE svtkRenderWindowInteractor
      OVERRIDE svtkCocoaRenderWindowInteractor)
  endif ()
  list(APPEND sources svtkCocoaRenderWindowInteractor.mm)
  list(APPEND headers
    svtkCocoaRenderWindowInteractor.h
    svtkCocoaMacOSXSDKCompatibility.h)
  # Set specified Objective-C++ flags, if any.
  if (SVTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(
      svtkCocoaRenderWindowInteractor.mm
      PROPERTIES
        COMPILE_FLAGS "${SVTK_REQUIRED_OBJCXX_FLAGS}")
  endif ()
elseif (ANDROID)
  # Add some custom overrides
  svtk_object_factory_declare(
    BASE svtkRenderWindowInteractor
    OVERRIDE svtkAndroidRenderWindowInteractor)
  list(APPEND classes svtkAndroidRenderWindowInteractor)
  include_directories("${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
elseif (APPLE_IOS)
  # Add some custom overrides
  svtk_object_factory_declare(
    BASE svtkRenderWindowInteractor
    OVERRIDE svtkIOSRenderWindowInteractor)
  list(APPEND sources svtkIOSRenderWindowInteractor.mm)
  list(APPEND headers svtkIOSRenderWindowInteractor.h)
  # Set specified Objective-C++ flags, if any.
  if (SVTK_REQUIRED_OBJCXX_FLAGS)
    set_source_files_properties(
      svtkIOSRenderWindowInteractor.mm
      PROPERTIES
        COMPILE_FLAGS "${SVTK_REQUIRED_OBJCXX_FLAGS}")
  endif ()
else () # use generic
  svtk_object_factory_declare(
    BASE svtkRenderWindowInteractor
    OVERRIDE svtkGenericRenderWindowInteractor)
endif ()

svtk_object_factory_configure(
  SOURCE_FILE svtk_object_factory_source
  HEADER_FILE svtk_object_factory_header
  EXPORT_MACRO "SVTKRENDERINGUI_EXPORT"
  )

svtk_module_add_module(SVTK::RenderingUI
  CLASSES ${classes}
  HEADERS ${headers}
  SOURCES ${sources} ${svtk_object_factory_source}
  PRIVATE_HEADERS ${svtk_object_factory_header}
  )

if (SVTK_USE_X)
  svtk_module_find_package(
    PACKAGE X11)
  svtk_module_link(SVTK::RenderingUI
    PUBLIC
      X11::X11)
elseif (SVTK_USE_COCOA)
  svtk_module_link(SVTK::RenderingUI PUBLIC "-framework Cocoa")
endif ()

if (ANDROID)
  svtk_module_link(SVTK::RenderingUI
    PRIVATE
      android)
endif ()

if (APPLE_IOS)
  svtk_module_link(SVTK::RenderingUI PUBLIC "-framework UIKit")
endif ()
