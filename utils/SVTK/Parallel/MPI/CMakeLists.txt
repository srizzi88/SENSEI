set(classes
  svtkMPICommunicator
  svtkMPIController
  svtkMPIUtilities)

set(headers
  svtkMPI.h)

svtk_module_add_module(SVTK::ParallelMPI
  CLASSES ${classes}
  HEADERS ${headers})
