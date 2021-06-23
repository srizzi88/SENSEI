# TODO: Make FindTCL.cmake use imported targets
find_package(TCL REQUIRED)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtkTkAppInitConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtkTkAppInitConfigure.h")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtkTkInternals.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtkTkInternals.h")

set(sources
  svtkPythonTkWidgetsInit.cxx
  svtkTkImageViewerWidgetPython.cxx
  svtkTkRenderWidgetPython.cxx)
set(private_headers
  svtkTcl.h
  svtkTclUtil.h
  svtkTk.h
  svtkTkImageViewerWidget.h
  svtkTkRenderWidget.h
  "${CMAKE_CURRENT_BINARY_DIR}/svtkTkAppInitConfigure.h"
  "${CMAKE_CURRENT_BINARY_DIR}/svtkTkInternals.h")

if (SVTK_USE_COCOA)
  list(APPEND sources
    svtkCocoaTkUtilities.mm)
  list(APPEND private_headers
    svtkCocoaTkUtilities.h)
endif ()

# Need Tk internal headers on Windows and OS X
set(_internal_tk_header)
set(_internal_tk_include_paths)
if (WIN32)
  get_filename_component(_parent "${TK_INCLUDE_PATH}" PATH)
  set(_internal_tk_header "tkWinInt.h")
  set(_internal_tk_include_paths "${_parent}/win")
elseif (SVTK_USE_COCOA)
  get_filename_component(_parent "${TK_INCLUDE_PATH}" PATH)
  set(_internal_tk_header "tkMacOSXInt.h")
  set(_internal_tk_include_paths
    "${_parent}/PrivateHeaders"
    "${_parent}/tk-private"
    "${_parent}/macosx")
endif ()

if (_internal_tk_header)
  find_path(
     TK_INTERNAL_PATH
     ${_internal_tk_header}
     PATHS ${_internal_tk_include_paths} ${TK_INCLUDE_PATH}
     DOC "The path to the Tk internal headers (${_internal_tk_header}).")
  mark_as_advanced(TK_INTERNAL_PATH)
else ()
  set(TK_INTERNAL_PATH)
endif ()

svtk_module_add_module(SVTK::RenderingTk
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
svtk_module_link(SVTK::RenderingTk
  PRIVATE
    ${TCL_LIBRARY}
    ${TK_LIBRARY})
svtk_module_include(SVTK::RenderingTk
  PRIVATE
    ${TCL_INCLUDE_PATH}
    ${TK_INCLUDE_PATH}
    ${TK_INTERNAL_PATH})
