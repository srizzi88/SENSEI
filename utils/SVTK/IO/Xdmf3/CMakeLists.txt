set(classes
  svtkXdmf3ArrayKeeper
  svtkXdmf3ArraySelection
  svtkXdmf3DataSet
  svtkXdmf3HeavyDataHandler
  svtkXdmf3LightDataHandler
  svtkXdmf3Reader
  svtkXdmf3SILBuilder
  svtkXdmf3Writer)

svtk_module_add_module(SVTK::IOXdmf3
  CLASSES ${classes})
