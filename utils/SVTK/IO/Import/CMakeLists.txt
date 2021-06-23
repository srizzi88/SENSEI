set(classes
  svtk3DSImporter
  svtkGLTFImporter
  svtkImporter
  svtkVRMLImporter
  svtkOBJImporter
  svtkOBJImporterInternals)

set(headers
  svtk3DS.h)

svtk_module_add_module(SVTK::IOImport
  CLASSES ${classes}
  HEADERS ${headers})
