set(classes
  svtkSILBuilder
  svtkXdmfDataArray
  svtkXdmfHeavyData
  svtkXdmfReader
  svtkXdmfReaderInternal
  svtkXdmfWriter)

svtk_module_add_module(SVTK::IOXdmf2
  CLASSES ${classes})
