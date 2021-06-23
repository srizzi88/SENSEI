include("${CMAKE_CURRENT_SOURCE_DIR}/SVTKMFCSettings.cmake")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/svtkMFCConfigure.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/svtkMFCConfigure.h"
  )

set(MFC_SRCS
  svtkMFCWindow.cpp
  )
set(MFC_HDRS
  svtkMFCWindow.h
  )

svtk_module_add_module(SVTK::GUISupportMFC
  SOURCES ${MFC_SRCS}
  HEADERS ${MFC_HDRS}
          "${CMAKE_CURRENT_BINARY_DIR}/svtkMFCConfigure.h")

if(SVTK_MFC_EXTRA_LIBS)
  svtk_module_link(SVTK::GUISupportMFC PRIVATE ${SVTK_MFC_EXTRA_LIBS})
endif()
