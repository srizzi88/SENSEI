set(sources
  gl2ps.c)

set(headers
  gl2ps.h
  svtk_gl2ps_mangle.h)

svtk_module_add_module(SVTK::gl2ps
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "svtkgl2ps")
set_target_properties(gl2ps
  PROPERTIES
    DEFINE_SYMBOL GL2PSDLL_EXPORTS)

if (BUILD_SHARED_LIBS)
  target_compile_definitions(gl2ps
    PRIVATE
      GL2PSDLL)
endif ()

target_compile_definitions(gl2ps
  PRIVATE
    HAVE_ZLIB
    HAVE_PNG)

if (UNIX)
  target_link_libraries(gl2ps
    PRIVATE
      m)
endif ()
