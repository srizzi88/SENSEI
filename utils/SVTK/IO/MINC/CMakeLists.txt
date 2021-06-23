set(classes
  svtkMINCImageAttributes
  svtkMINCImageReader
  svtkMINCImageWriter
  svtkMNIObjectReader
  svtkMNIObjectWriter
  svtkMNITagPointReader
  svtkMNITagPointWriter
  svtkMNITransformReader
  svtkMNITransformWriter)

set(headers
  svtkMINC.h)

svtk_module_add_module(SVTK::IOMINC
  CLASSES ${classes}
  HEADERS ${headers})
