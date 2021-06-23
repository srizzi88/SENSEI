set(classes
  svtkODBCDatabase
  svtkODBCQuery)

svtk_module_add_module(SVTK::IOODBC
  CLASSES ${classes}
  SOURCES svtkIOODBC_AutoInit.cxx)

svtk_module_find_package(PACKAGE ODBC)
svtk_module_link(SVTK::IOODBC
  PRIVATE
    ODBC::ODBC)
