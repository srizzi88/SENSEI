include(../../../../CMake/svtkGhostscript.cmake)

if(SVTK_GHOSTSCRIPT_EXECUTABLE)
  set(PDFTests
    TestPDFContextExport.cxx,NO_DATA
    TestPDFContextShading.cxx,NO_DATA
    TestPDFExporterContextPolyData.cxx
    TestPDFTransformedText.cxx,NO_DATA
  )

  svtk_add_test_cxx(svtkIOExportPDFCxxTests tests
    ${PDFTests}
    )
  svtk_test_cxx_executable(svtkIOExportPDFCxxTests tests
    RENDERING_FACTORY
    )

  include(../../../../CMake/svtkTestingPDF.cmake)
  svtk_add_pdf_test_cxx(${PDFTests})
endif()
