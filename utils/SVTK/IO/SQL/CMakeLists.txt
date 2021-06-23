set(classes
  svtkDatabaseToTableReader
  svtkRowQuery
  svtkRowQueryToTable
  svtkSQLDatabase
  #svtkSQLDatabaseGraphSource
  svtkSQLDatabaseSchema
  svtkSQLDatabaseTableSource
  #svtkSQLGraphReader
  svtkSQLiteDatabase
  svtkSQLiteQuery
  svtkSQLiteToTableReader
  svtkSQLQuery
  svtkTableToDatabaseWriter
  svtkTableToSQLiteWriter)

set(private_headers
  svtkSQLiteDatabaseInternals.h)

svtk_module_add_module(SVTK::IOSQL
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})
