set(classes
  svtkMPASReader
  svtkNetCDFCAMReader
  svtkNetCDFCFReader
  svtkNetCDFPOPReader
  svtkNetCDFReader
  svtkSLACParticleReader
  svtkSLACReader)

svtk_module_add_module(SVTK::IONetCDF
  CLASSES ${classes})
