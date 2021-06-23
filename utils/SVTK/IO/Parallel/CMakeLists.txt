set(classes
  svtkEnSightWriter
  svtkMultiBlockPLOT3DReader
  svtkPlot3DMetaReader
  svtkPChacoReader
  svtkPDataSetReader
  svtkPDataSetWriter
  svtkPImageWriter
  svtkPOpenFOAMReader)

set(sources
  svtkMultiBlockPLOT3DReaderInternals.cxx)

set(private_headers
  svtkMultiBlockPLOT3DReaderInternals.h)

svtk_module_add_module(SVTK::IOParallel
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
