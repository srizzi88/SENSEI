if (NOT DEFINED KISSFFT_SCALAR_TYPE)
  set(KISSFFT_SCALAR_TYPE "double")
endif ()

set(sources
  kiss_fft.c
  tools/kiss_fastfir.c
  tools/kiss_fftnd.c
  tools/kiss_fftndr.c
  tools/kiss_fftr.c)

set(headers
  kiss_fft_exports.h
  _kiss_fft_guts.h
  kiss_fft.h
  tools/kfc.h
  tools/kiss_fftnd.h
  tools/kiss_fftndr.h
  tools/kiss_fftr.h
  tools/svtkkissfft_fftnd.h
  tools/svtkkissfft_fftndr.h
  tools/svtkkissfft_fftr.h
  svtkkissfft_fft.h
  svtk_kiss_fft_mangle.h)
svtk_module_add_module(SVTK::kissfft
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "svtkkissfft")

target_compile_definitions(kissfft
  PUBLIC "kiss_fft_scalar=${KISSFFT_SCALAR_TYPE}")
if (WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(kissfft
    PRIVATE
      KISSFFT_DLL_EXPORT=1
    INTERFACE
      KISSFFT_DLL_IMPORT=1)
endif ()
target_include_directories(kissfft
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
