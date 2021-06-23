set(classes
  svtkVtkJSSceneGraphSerializer
  svtkVtkJSViewNodeFactory
)

svtk_module_add_module(SVTK::RenderingVtkJS
  CLASSES ${classes})
