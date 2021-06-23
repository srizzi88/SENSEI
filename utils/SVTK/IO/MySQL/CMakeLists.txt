set(classes
  svtkMySQLDatabase
  svtkMySQLQuery
  svtkMySQLToTableReader
  svtkTableToMySQLWriter)

set(sources
  svtkIOMySQL_AutoInit.cxx)

svtk_module_add_module(SVTK::IOMySQL
  CLASSES ${classes}
  SOURCES ${sources})

svtk_module_find_package(PACKAGE MySQL)
svtk_module_link(SVTK::IOMySQL
  PRIVATE
    MySQL::MySQL)
