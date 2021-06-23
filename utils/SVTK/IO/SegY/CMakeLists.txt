set(classes
  svtkSegYReader)

set(sources
  svtkSegYIOUtils.cxx
  svtkSegYReaderInternal.cxx
  svtkSegYTraceReader.cxx)

set(private_headers
  svtkSegYIOUtils.h
  svtkSegYReaderInternal.h
  svtkSegYTraceReader.h)

svtk_module_add_module(SVTK::IOSegY
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})
