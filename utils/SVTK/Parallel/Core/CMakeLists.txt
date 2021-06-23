set(classes
  svtkCommunicator
  svtkDummyCommunicator
  svtkDummyController
  svtkFieldDataSerializer
  svtkMultiProcessController
  svtkMultiProcessStream
  svtkPDirectory
  svtkProcess
  svtkProcessGroup
  svtkPSystemTools
  svtkSocketCommunicator
  svtkSocketController
  svtkSubCommunicator
  svtkSubGroup)

set(template_classes
  svtkThreadedTaskQueue)

include(svtkHashSource)
# Generate "svtkSocketCommunicatorHash.h".
svtk_hash_source(
  INPUT "svtkSocketCommunicator.cxx"
  NAME "svtkSocketCommunicatorHash"
  HEADER_OUTPUT hash_header)

svtk_module_add_module(SVTK::ParallelCore
  CLASSES ${classes}
  TEMPLATE_CLASSES  ${template_classes}
  PRIVATE_HEADERS ${hash_header})
