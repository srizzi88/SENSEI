set(classes
  svtkQImageToImageSource
  svtkQtInitialization
  svtkQtLabelRenderStrategy
  svtkQtStringToImage
  svtkQtTreeRingLabelMapper
  )

svtk_module_add_module(SVTK::RenderingQt
  CLASSES ${classes})

svtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  Widgets
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt5_VERSION)
svtk_module_link(SVTK::RenderingQt PUBLIC Qt5::Widgets)
