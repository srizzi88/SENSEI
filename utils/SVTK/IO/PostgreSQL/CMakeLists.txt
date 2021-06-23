svtk_module_find_package(PACKAGE PostgreSQL)

set(classes
  svtkPostgreSQLDatabase
  svtkPostgreSQLQuery
  svtkPostgreSQLToTableReader
  svtkTableToPostgreSQLWriter)

svtk_module_add_module(SVTK::IOPostgreSQL
  CLASSES ${classes}
  SOURCES svtkIOPostgreSQL_AutoInit.cxx)

svtk_module_link(SVTK::IOPostgreSQL
  PRIVATE
    PostgreSQL::PostgreSQL)
