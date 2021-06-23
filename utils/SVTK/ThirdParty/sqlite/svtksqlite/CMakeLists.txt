svtk_module_add_module(SVTK::sqlite
  SOURCES sqlite3.c
  HEADERS sqlite3.h
          svtk_sqlite_mangle.h
          "${CMAKE_CURRENT_BINARY_DIR}/svtksqlite_export.h"
  HEADERS_SUBDIR "svtksqlite")

target_compile_definitions(sqlite
  PRIVATE
    SQLITE_THREADSAFE=1)
target_link_libraries(sqlite
  PRIVATE
    ${CMAKE_THREAD_LIBS})
target_include_directories(sqlite
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>")

include(GenerateExportHeader)
generate_export_header(sqlite
  EXPORT_MACRO_NAME SQLITE_API
  EXPORT_FILE_NAME  svtksqlite_export.h
  DEPRECATED_MACRO_NAME SVTKSQLITE_DEPRECATED)
