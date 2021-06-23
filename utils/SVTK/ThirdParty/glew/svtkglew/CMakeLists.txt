set(sources
  src/glew.c)

set(headers
  include/GL/glew.h
  include/GL/glxew.h
  include/GL/svtk_glew_mangle.h
  include/GL/wglew.h)

svtk_module_add_module(SVTK::glew
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "svtkglew/include/GL")
target_compile_definitions(glew
  PRIVATE
    GLEW_NO_GLU)


if(SVTK_OPENGL_HAS_OSMESA AND UNIX)
  target_compile_definitions(glew
    PRIVATE
      GLEW_OSMESA)
  target_link_libraries(glew
    PRIVATE
      ${CMAKE_DL_LIBS})
endif()

if(SVTK_OPENGL_HAS_EGL)
  target_compile_definitions(glew
    PRIVATE
      GLEW_EGL)
endif()

if (BUILD_SHARED_LIBS)
  if (WIN32)
    target_compile_definitions(glew
      PRIVATE
        GLEW_BUILD)
    if(MINGW)
      # https://github.com/nigels-com/glew/issues/157
      target_link_libraries(glew
        PRIVATE
          -nostdlib)
    endif()
  endif ()
else ()
  target_compile_definitions(glew
    PUBLIC
      GLEW_STATIC)
endif ()

target_include_directories(glew
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
