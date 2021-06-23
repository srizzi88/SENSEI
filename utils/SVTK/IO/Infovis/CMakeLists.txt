set(classes
  svtkBiomTableReader
  svtkChacoGraphReader
  svtkDelimitedTextReader
  svtkDIMACSGraphReader
  svtkDIMACSGraphWriter
  svtkFixedWidthTextReader
  svtkISIReader
  svtkMultiNewickTreeReader
  svtkNewickTreeReader
  svtkNewickTreeWriter
  svtkPhyloXMLTreeReader
  svtkPhyloXMLTreeWriter
  svtkRISReader
  svtkTemporalDelimitedTextReader
  svtkTulipReader
  svtkXGMLReader
  svtkXMLTreeReader)

svtk_module_add_module(SVTK::IOInfovis
  CLASSES ${classes})
