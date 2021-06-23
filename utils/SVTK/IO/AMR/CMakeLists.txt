set(classes
  svtkAMRBaseParticlesReader
  svtkAMRBaseReader
  svtkAMRDataSetCache
  svtkAMREnzoParticlesReader
  svtkAMREnzoReader
  svtkAMRFlashParticlesReader
  svtkAMRFlashReader
  svtkAMReXParticlesReader
  svtkAMReXGridReader)

set(sources
  svtkAMREnzoReaderInternal.cxx
  svtkAMRFlashReaderInternal.cxx
  svtkAMReXGridReaderInternal.cxx)

set(private_headers
  svtkAMREnzoReaderInternal.h
  svtkAMRFlashReaderInternal.h
  svtkAMReXGridReaderInternal.h)

svtk_module_add_module(SVTK::IOAMR
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
