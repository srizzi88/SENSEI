cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(QtSVTKTouchscreenRenderWindows)

find_package(SVTK
  COMPONENTS
    CommonCore
    GUISupportQt
    IOImage
    ImagingColor
    ImagingGeneral
    InteractionImage
    InteractionStyle
    InteractionWidgets)
if (NOT SVTK_FOUND)
  message("Skipping example: ${SVTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

find_package(Qt5 COMPONENTS Widgets)
if (NOT TARGET Qt5::Widgets)
  message("Skipping example: ${Qt5_NOT_FOUND_MESSAGE}")
  return ()
endif ()

# Set your files and resources here
set(Srcs
  QtSVTKTouchscreenRenderWindowsApp.cxx
  QtSVTKTouchscreenRenderWindows.cxx)

set(Hdrs
  QtSVTKTouchscreenRenderWindows.h)

set(UIs
  QtSVTKTouchscreenRenderWindows.ui)

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# CMAKE_AUTOMOC in ON so the MocHdrs will be automatically wrapped.
qt5_wrap_ui(UI_Srcs ${UIs})
add_executable(QtSVTKTouchscreenRenderWindows
  ${Srcs} ${Hdrs} ${UI_Srcs})
target_link_libraries(QtSVTKTouchscreenRenderWindows
  PRIVATE
    ${SVTK_LIBRARIES}
    Qt5::Widgets)
svtk_module_autoinit(
  TARGETS QtSVTKTouchscreenRenderWindows
  MODULES ${SVTK_LIBRARIES})
