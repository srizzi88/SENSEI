cmake_minimum_required(VERSION 3.8...3.12 FATAL_ERROR)
project(QtSVTKRenderWindows)

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
  QtSVTKRenderWindowsApp.cxx
  QtSVTKRenderWindows.cxx)

set(Hdrs
  QtSVTKRenderWindows.h)

set(UIs
  QtSVTKRenderWindows.ui)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# CMAKE_AUTOMOC in ON so the MocHdrs will be automatically wrapped.
add_executable(QtSVTKRenderWindows
  ${Srcs} ${Hdrs} ${UIs})
target_link_libraries(QtSVTKRenderWindows
  PRIVATE
    ${SVTK_LIBRARIES}
    Qt5::Widgets)
svtk_module_autoinit(
  TARGETS QtSVTKRenderWindows
  MODULES ${SVTK_LIBRARIES})
