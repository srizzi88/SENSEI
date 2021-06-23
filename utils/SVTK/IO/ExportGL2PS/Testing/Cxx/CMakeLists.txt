include(../../../../CMake/svtkGhostscript.cmake)

# Only run GL2PS tests if GhostScript is available
if(SVTK_GHOSTSCRIPT_EXECUTABLE)
  set(GL2PSTests
    TestGL2PSBillboardTextActor3D.cxx
    TestGL2PSContext.cxx # No image export supported in OGL
    TestGL2PSLabeledDataMapper.cxx
    TestGL2PSScalarBar.cxx
    TestGL2PSTextActor3D.cxx
    TestGL2PSTextActor.cxx
    TestGL2PSTextMapper.cxx
    TestStackedPlotGL2PS.cxx
    )

  set(GL2PSTestsPDFBothOpenGL
    TestGL2PSExporterMultipleRenderers.cxx
    TestGL2PSExporterRaster.cxx
    TestGL2PSExporterVolumeRaster.cxx
    )

  set(GL2PSTestsPDF
    ${GL2PSTestsPDFBothOpenGL}
  )

  svtk_add_test_cxx(svtkIOExportGL2PSCxxTests tests
    ${GL2PSTests}
    ${GL2PSTestsPDF}
    )
  svtk_test_cxx_executable(svtkIOExportGL2PSCxxTests tests RENDERING_FACTORY)

  set(TestGL2PSExporterVolumeRasterError 50)

  include(../../../../CMake/svtkTestingGL2PS.cmake)
  # text alignment and rotation with TextAsPath off is not
  # yet implemented for the OpenGL2 backend.
  svtk_add_gl2ps_test_cxx(${GL2PSTests})
  # test postscript and pdf
  svtk_add_gl2ps_test_cxx(pdf ${GL2PSTestsPDFBothOpenGL})
endif()
