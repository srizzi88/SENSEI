set(classes
  svtkActorNode
  svtkCameraNode
  svtkLightNode
  svtkMapperNode
  svtkPolyDataMapperNode
  svtkRendererNode
  svtkViewNode
  svtkViewNodeCollection
  svtkViewNodeFactory
  svtkVolumeMapperNode
  svtkVolumeNode
  svtkWindowNode)

svtk_module_add_module(SVTK::RenderingSceneGraph
  CLASSES ${classes})
