set(classes
  svtkAVSucdReader
  svtkBYUReader
  svtkBYUWriter
  #svtkCGMWriter # Needs svtkViewport.
  svtkChacoReader
  svtkFacetWriter
  svtkFLUENTReader
  svtkGAMBITReader
  svtkGaussianCubeReader
  svtkGLTFDocumentLoader
  svtkGLTFReader
  svtkHoudiniPolyDataWriter
  svtkIVWriter
  svtkMCubesReader
  svtkMCubesWriter
  svtkMFIXReader
  svtkMoleculeReaderBase
  svtkOBJReader
  svtkOBJWriter
  svtkOpenFOAMReader
  svtkParticleReader
  svtkPDBReader
  svtkProStarReader
  svtkPTSReader
  svtkSTLReader
  svtkSTLWriter
  svtkTecplotReader
  svtkWindBladeReader
  svtkXYZMolReader)

set(private_classes
  svtkGLTFDocumentLoaderInternals
  svtkGLTFUtils)

svtk_module_add_module(SVTK::IOGeometry
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  )
