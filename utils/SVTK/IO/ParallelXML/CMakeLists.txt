set(classes
  svtkXMLPDataSetWriter
  svtkXMLPDataObjectWriter
  svtkXMLPDataWriter
  svtkXMLPImageDataWriter
  svtkXMLPPolyDataWriter
  svtkXMLPRectilinearGridWriter
  svtkXMLPStructuredDataWriter
  svtkXMLPStructuredGridWriter
  svtkXMLPTableWriter
  svtkXMLPUnstructuredDataWriter
  svtkXMLPUnstructuredGridWriter
  svtkXMLPHierarchicalBoxDataWriter
  svtkXMLPMultiBlockDataWriter
  svtkXMLPUniformGridAMRWriter
  svtkXMLPHyperTreeGridWriter)

svtk_module_add_module(SVTK::IOParallelXML
  CLASSES ${classes})
