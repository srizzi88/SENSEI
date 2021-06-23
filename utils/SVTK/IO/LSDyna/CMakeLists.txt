set(classes
  LSDynaFamily
  LSDynaMetaData
  svtkLSDynaPart
  svtkLSDynaPartCollection
  svtkLSDynaReader
  svtkLSDynaSummaryParser)

svtk_module_add_module(SVTK::IOLSDyna
  CLASSES ${classes}
  SOURCES ${sources})
