# Allow the user to customize their build with some local options
include("${CMAKE_CURRENT_SOURCE_DIR}/LocalUserOptions.cmake" OPTIONAL)

set(ns "SVTK")
if (SVTK_BUILD_COMPILE_TOOLS_ONLY)
  set(ns "SVTKCompileTools")
endif ()

set(sources
  svtkParse.tab.c
  svtkParseData.c
  svtkParseExtras.c
  svtkParseHierarchy.c
  svtkParseMain.c
  svtkParseMangle.c
  svtkParseMerge.c
  svtkParsePreprocess.c
  svtkParseString.c
  svtkWrap.c
  svtkWrapText.c)

set(headers
  svtkParse.h
  svtkParseData.h
  svtkParseExtras.h
  svtkParseHierarchy.h
  svtkParseMain.h
  svtkParseMangle.h
  svtkParseMerge.h
  svtkParsePreprocess.h
  svtkParseString.h
  svtkParseType.h
  svtkWrap.h)

set_property(SOURCE svtkParseMain.c
  PROPERTY
    COMPILE_DEFINITIONS "SVTK_PARSE_VERSION=\"${SVTK_MAJOR_VERSION}.${SVTK_MINOR_VERSION}\"")

svtk_module_add_module("${ns}::WrappingTools"
  SOURCES ${sources}
  HEADERS ${headers})

if (NOT TARGET "${ns}::WrapHierarchy")
  svtk_module_add_executable(WrapHierarchy
    DEVELOPMENT
    BASENAME svtkWrapHierarchy
    svtkWrapHierarchy.c)
  target_link_libraries(WrapHierarchy
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::WrapHierarchy" ALIAS
    WrapHierarchy)
endif ()

if (NOT TARGET WrapPython)
  svtk_module_add_executable(WrapPython
    DEVELOPMENT
    BASENAME svtkWrapPython
    svtkWrapPython.c
    svtkWrapPythonClass.c
    svtkWrapPythonConstant.c
    svtkWrapPythonEnum.c
    svtkWrapPythonMethod.c
    svtkWrapPythonMethodDef.c
    svtkWrapPythonNamespace.c
    svtkWrapPythonOverload.c
    svtkWrapPythonTemplate.c
    svtkWrapPythonType.c)
  target_link_libraries(WrapPython
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::WrapPython" ALIAS
    WrapPython)

  svtk_module_add_executable(WrapPythonInit
    DEVELOPMENT
    BASENAME svtkWrapPythonInit
    svtkWrapPythonInit.c)
  add_executable("${ns}::WrapPythonInit" ALIAS
    WrapPythonInit)
endif ()

if (NOT TARGET WrapJava)
  svtk_module_add_executable(ParseJava
    DEVELOPMENT
    BASENAME svtkParseJava
    svtkParseJava.c)
  target_link_libraries(ParseJava
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::ParseJava" ALIAS
    ParseJava)

  svtk_module_add_executable(WrapJava
    DEVELOPMENT
    BASENAME svtkWrapJava
    svtkWrapJava.c)
  target_link_libraries(WrapJava
    PRIVATE
      "${ns}::WrappingTools")
  add_executable("${ns}::WrapJava" ALIAS
    WrapJava)
endif ()
