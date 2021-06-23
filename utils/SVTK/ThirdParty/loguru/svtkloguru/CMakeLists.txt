set(headers
  loguru.hpp
  "${CMAKE_CURRENT_BINARY_DIR}/svtkloguru_export.h")

set(sources
  loguru.cpp)

svtk_module_add_module(SVTK::loguru
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "svtkloguru")

include(GenerateExportHeader)
generate_export_header(loguru
  EXPORT_MACRO_NAME LOGURU_EXPORT
  EXPORT_FILE_NAME svtkloguru_export.h)

svtk_module_find_package(PACKAGE Threads)
svtk_module_link(SVTK::loguru PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
