svtk_add_test_cxx(svtkIOInfovisCxxTests tests
  NO_VALID
  TestBiomTableReader.cxx
  TestDIMACSGraphReader.cxx
  TestDataObjectIO.cxx
  TestISIReader.cxx
  TestFixedWidthTextReader.cxx
  TestNewickTreeReader.cxx
  TestNewickTreeWriter.cxx
  TestMultiNewickTreeReader.cxx
  TestPhyloXMLTreeReadWrite.cxx
  TestRISReader.cxx
  TestTulipReaderProperties.cxx
  TestDelimitedTextReader2.cxx
  TestTemporalDelimitedTextReader.cxx
  )
svtk_test_cxx_executable(svtkIOInfovisCxxTests tests)
