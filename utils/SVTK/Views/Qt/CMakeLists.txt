set(classes
  svtkQtAnnotationView
  svtkQtListView
  svtkQtRecordView
  svtkQtTableRepresentation
  svtkQtTableView
  svtkQtTreeView
  svtkQtView)

svtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  Widgets
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt5_VERSION)
set(CMAKE_AUTOMOC 1)

svtk_module_add_module(SVTK::ViewsQt
  CLASSES ${classes})

svtk_module_link(SVTK::ViewsQt PRIVATE Qt5::Widgets)
