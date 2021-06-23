PROJECT (Win32Cone)

IF(NOT SVTK_BINARY_DIR)
FIND_PACKAGE(SVTK)
IF(NOT SVTK_DIR)
  MESSAGE(FATAL_ERROR "Please set SVTK_DIR.")
ENDIF()
INCLUDE(${SVTK_USE_FILE})
ENDIF()

LINK_LIBRARIES(
  svtkRendering
  svtkGraphics
  svtkImaging
  svtkIO
  svtkFiltering
  svtkCommon
)

ADD_EXECUTABLE(Win32Cone WIN32 Win32Cone.cxx)
