set(classes
  svtkXMLDataParser
  svtkXMLParser
  svtkXMLUtilities)

svtk_module_add_module(SVTK::IOXMLParser
  CLASSES ${classes})
