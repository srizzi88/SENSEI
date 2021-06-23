# Include this file from a CMakeLists.txt that contains an
# SVTK_ADD_EXECUTABLE or SVTK_ADD_LIBRARY call when the build product
# links against MFC.
#
# This file provides definitions for the following CMake vars:
#   SVTK_MFC_STATIC
#   SVTK_MFC_LIB_TYPE
#   SVTK_MFC_DELAYLOAD_SVTK_DLLS
#   SVTK_MFC_EXTRA_LIBS
#
# It also automatically sets CMAKE_MFC_FLAG based on SVTK_MFC_STATIC
# and adds the _AFXDLL definition when linking to MFC dll.
#
# After including this file, use something like this chunk:
#   IF(SVTK_MFC_DELAYLOAD_SVTK_DLLS)
#     SVTK_MFC_ADD_DELAYLOAD_FLAGS(CMAKE_EXE_LINKER_FLAGS
#       svtkRendering.dll svtkFiltering.dll svtkCommon.dll
#     )
#   ENDIF()
# And then after your ADD_EXECUTABLE or ADD_LIBRARY:
#   IF(SVTK_MFC_EXTRA_LIBS)
#     TARGET_LINK_LIBRARIES(yourLibOrExe ${SVTK_MFC_EXTRA_LIBS})
#   ENDIF()
#
# If you're building an exe, use CMAKE_EXE_LINKER_FLAGS...
# If building a dll, use CMAKE_SHARED_LINKER_FLAGS instead...


# C runtime lib linkage and MFC lib linkage *MUST* match.
# If linking to C runtime static lib, link to MFC static lib.
# If linking to C runtime dll, link to MFC dll.
#
# If "/MT" or "/MTd" is in the compiler flags, then our build
# products will be linked to the C runtime static lib. Otherwise,
# to the C runtime dll.
#
IF(NOT DEFINED SVTK_MFC_STATIC)
  SET(SVTK_MFC_STATIC OFF)
  IF("${CMAKE_CXX_FLAGS_RELEASE}" MATCHES "/MT")
    SET(SVTK_MFC_STATIC ON)
  ENDIF()
ENDIF()

IF(SVTK_MFC_STATIC)
  # Tell CMake to link to MFC static lib
  SET(SVTK_MFC_LIB_TYPE "STATIC")
  SET(CMAKE_MFC_FLAG 1)
ELSE()
  # Tell CMake to link to MFC dll
  SET(SVTK_MFC_LIB_TYPE "SHARED")
  SET(CMAKE_MFC_FLAG 2)
  ADD_DEFINITIONS(-D_AFXDLL)
ENDIF()


# Using the linker /DELAYLOAD flag is necessary when SVTK is built
# as dlls to avoid false mem leak reporting by MFC shutdown code.
# Without it, the SVTK dlls load before the MFC dll at startup and
# unload after the MFC dll unloads at shutdown. Hence, any SVTK objects
# left at MFC dll unload time get reported as leaks.
#
IF(NOT DEFINED SVTK_MFC_DELAYLOAD_SVTK_DLLS)
  SET(SVTK_MFC_DELAYLOAD_SVTK_DLLS OFF)
  IF(SVTK_BUILD_SHARED_LIBS)
    SET(SVTK_MFC_DELAYLOAD_SVTK_DLLS ON)
  ENDIF()
ENDIF()

IF(NOT DEFINED SVTK_MFC_EXTRA_LIBS)
  SET(SVTK_MFC_EXTRA_LIBS)
ENDIF()

IF(SVTK_MFC_STATIC)
  SET(SVTK_MFC_EXTRA_LIBS ${SVTK_MFC_EXTRA_LIBS}
    debug nafxcwd optimized nafxcw
    debug LIBCMTD optimized LIBCMT
    Uxtheme windowscodecs
    )
ENDIF()

IF(SVTK_MFC_DELAYLOAD_SVTK_DLLS)
  SET(SVTK_MFC_EXTRA_LIBS ${SVTK_MFC_EXTRA_LIBS} "DelayImp")
ENDIF()

MACRO(SVTK_MFC_ADD_DELAYLOAD_FLAGS flagsVar)
  SET(dlls "${ARGN}")
  FOREACH(dll ${dlls})
    SET(${flagsVar} "${${flagsVar}} /DELAYLOAD:${dll}")
  ENDFOREACH()
ENDMACRO()
