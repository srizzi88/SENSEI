set(classes
  svtkAbstractParticleWriter
  svtkAbstractPolyDataReader
  svtkArrayDataReader
  svtkArrayDataWriter
  svtkArrayReader
  svtkArrayWriter
  svtkASCIITextCodec
  svtkBase64InputStream
  svtkBase64OutputStream
  svtkBase64Utilities
  svtkDataCompressor
  svtkDelimitedTextWriter
  svtkGlobFileNames
  svtkInputStream
  svtkJavaScriptDataWriter
  svtkLZ4DataCompressor
  svtkLZMADataCompressor
  svtkNumberToString
  svtkOutputStream
  svtkSortFileNames
  svtkTextCodec
  svtkTextCodecFactory
  svtkUTF16TextCodec
  svtkUTF8TextCodec
  svtkWriter
  svtkZLibDataCompressor)

set(headers
  svtkUpdateCellsV8toV9.h)

svtk_module_add_module(SVTK::IOCore
  CLASSES ${classes}
  HEADERS ${headers})
