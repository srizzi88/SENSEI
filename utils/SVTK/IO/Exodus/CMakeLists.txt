set(classes
  svtkCPExodusIIElementBlock
  svtkCPExodusIIInSituReader
  svtkExodusIICache
  svtkExodusIIReader
  svtkExodusIIReaderParser
  svtkExodusIIReaderVariableCheck
  svtkExodusIIWriter
  svtkModelMetadata)

set(template_classes
  svtkCPExodusIINodalCoordinatesTemplate
  svtkCPExodusIIResultsArrayTemplate)

set(headers
  svtkExodusIIReaderPrivate.h)

svtk_module_add_module(SVTK::IOExodus
  CLASSES ${classes}
  TEMPLATE_CLASSES ${template_classes}
  HEADERS ${headers})
