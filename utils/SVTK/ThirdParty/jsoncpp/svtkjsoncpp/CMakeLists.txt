set(sources
  jsoncpp.cpp)

set(headers
  json/json-forwards.h
  json/json.h
  "${CMAKE_CURRENT_BINARY_DIR}/json/svtkjsoncpp_config.h")

if (BUILD_SHARED_LIBS)
  set(JSON_DLL 1)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/json/svtkjsoncpp_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/json/svtkjsoncpp_config.h")

svtk_module_add_module(SVTK::jsoncpp
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR svtkjsoncpp/json)
if (SVTK_REQUIRE_LARGE_FILE_SUPPORT)
  target_compile_definitions(jsoncpp
    PRIVATE
      _LARGEFILE_SOURCE
      _LARGEFILE64_SOURCE
      _LARGE_FILES
      _FILE_OFFSET_BITS=64)
endif ()
target_include_directories(jsoncpp
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/json>")
target_compile_features(jsoncpp
  PUBLIC
    cxx_std_11)
set_target_properties(jsoncpp
  PROPERTIES
    DEFINE_SYMBOL JSON_DLL_BUILD)
