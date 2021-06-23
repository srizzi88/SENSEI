# set up sources to build
set(classes
  svtkQtSQLDatabase
  svtkQtSQLQuery
  svtkQtTimePointUtility
  )

svtk_module_add_module(SVTK::GUISupportQtSQL
  CLASSES ${classes})

svtk_module_find_package(
  PACKAGE     Qt5
  COMPONENTS  Widgets Sql
  FORWARD_VERSION_REQ MINOR
  VERSION_VAR         Qt5_VERSION)
svtk_module_link(SVTK::GUISupportQtSQL PUBLIC Qt5::Sql Qt5::Widgets)
