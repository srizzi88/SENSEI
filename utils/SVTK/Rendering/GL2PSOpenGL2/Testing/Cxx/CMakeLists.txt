include(../../../../CMake/svtkTestingGL2PS.cmake)

set(GL2PSTests)
if(SVTK_GHOSTSCRIPT_EXECUTABLE)
  set(GL2PSTests
    TestGL2PSAddPolyPrimitive.cxx
  )
endif()

svtk_add_test_cxx(svtkRenderingGL2PSOpenGL2CxxTests tests NO_VALID ${GL2PSTests})

# Create rasterize/verify tests to check generated postscript:
svtk_add_gl2ps_test_cxx(${GL2PSTests})

# Compile the PNGCompare test, which is used to validate PNG images (read from
# disk, not generated in SVTK).
list(APPEND tests PNGCompare)

svtk_test_cxx_executable(svtkRenderingGL2PSOpenGL2CxxTests tests RENDERING_FACTORY)
