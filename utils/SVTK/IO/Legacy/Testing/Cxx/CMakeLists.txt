svtk_add_test_cxx(svtkIOLegacyCxxTests tests
  TestLegacyCompositeDataReaderWriter.cxx,NO_VALID
  TestLegacyGhostCellsImport.cxx
  TestLegacyArrayMetaData.cxx,NO_VALID
  )
svtk_test_cxx_executable(svtkIOLegacyCxxTests tests
    RENDERING_FACTORY
    )
