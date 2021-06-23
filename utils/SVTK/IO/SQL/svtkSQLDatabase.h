/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkSQLDatabase.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkSQLDatabase
 * @brief   maintain a connection to an sql database
 *
 *
 * Abstract base class for all SQL database connection classes.
 * Manages a connection to the database, and is responsible for creating
 * instances of the associated svtkSQLQuery objects associated with this
 * class in order to perform execute queries on the database.
 * To allow connections to a new type of database, create both a subclass
 * of this class and svtkSQLQuery, and implement the required functions:
 *
 * Open() - open the database connection, if possible.
 * Close() - close the connection.
 * GetQueryInstance() - create and return an instance of the svtkSQLQuery
 *                      subclass associated with the database type.
 *
 * The subclass should also provide API to set connection parameters.
 *
 * This class also provides the function EffectSchema to transform a
 * database schema into a SQL database.
 *
 * @par Thanks:
 * Thanks to Andrew Wilson from Sandia National Laboratories for his work
 * on the database classes and for the SQLite example. Thanks to David Thompson
 * and Philippe Pebay from Sandia National Laboratories for implementing
 * this class.
 *
 * @sa
 * svtkSQLQuery
 * svtkSQLDatabaseSchema
 */

#ifndef svtkSQLDatabase_h
#define svtkSQLDatabase_h

#include "svtkIOSQLModule.h" // For export macro
#include "svtkObject.h"

#include "svtkStdString.h" // Because at least one method returns a svtkStdString

class svtkInformationObjectBaseKey;
class svtkSQLDatabaseSchema;
class svtkSQLQuery;
class svtkStringArray;

// This is a list of features that each database may or may not
// support.  As yet (April 2008) we don't provide access to most of
// them.
#define SVTK_SQL_FEATURE_TRANSACTIONS 1000
#define SVTK_SQL_FEATURE_QUERY_SIZE 1001
#define SVTK_SQL_FEATURE_BLOB 1002
#define SVTK_SQL_FEATURE_UNICODE 1003
#define SVTK_SQL_FEATURE_PREPARED_QUERIES 1004
#define SVTK_SQL_FEATURE_NAMED_PLACEHOLDERS 1005
#define SVTK_SQL_FEATURE_POSITIONAL_PLACEHOLDERS 1006
#define SVTK_SQL_FEATURE_LAST_INSERT_ID 1007
#define SVTK_SQL_FEATURE_BATCH_OPERATIONS 1008
#define SVTK_SQL_FEATURE_TRIGGERS 1009 // supported

// Default size for columns types which require a size to be specified
// (i.e., VARCHAR), when no size has been specified
#define SVTK_SQL_DEFAULT_COLUMN_SIZE 32

class SVTKIOSQL_EXPORT svtkSQLDatabase : public svtkObject
{
public:
  svtkTypeMacro(svtkSQLDatabase, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Open a new connection to the database.
   * You need to set up any database parameters before calling this function.
   * For database connections that do not require a password, pass an empty string.
   * Returns true is the database was opened successfully, and false otherwise.
   */
  virtual bool Open(const char* password) = 0;

  /**
   * Close the connection to the database.
   */
  virtual void Close() = 0;

  /**
   * Return whether the database has an open connection.
   */
  virtual bool IsOpen() = 0;

  /**
   * Return an empty query on this database.
   */
  virtual SVTK_NEWINSTANCE svtkSQLQuery* GetQueryInstance() = 0;

  /**
   * Did the last operation generate an error
   */
  virtual bool HasError() = 0;

  /**
   * Get the last error text from the database
   * I'm using const so that people do NOT
   * use the standard svtkGetStringMacro in their
   * implementation, because 99% of the time that
   * will not be the correct thing to do...
   */
  virtual const char* GetLastErrorText() = 0;

  /**
   * Get the type of the database (e.g. mysql, psql,..).
   */
  virtual const char* GetDatabaseType() = 0;

  /**
   * Get the list of tables from the database.
   */
  virtual svtkStringArray* GetTables() = 0;

  /**
   * Get the list of fields for a particular table.
   */
  virtual svtkStringArray* GetRecord(const char* table) = 0;

  /**
   * Return whether a feature is supported by the database.
   */
  virtual bool IsSupported(int svtkNotUsed(feature)) { return false; }

  /**
   * Get the URL of the database.
   */
  virtual svtkStdString GetURL() = 0;

  /**
   * Return the SQL string with the syntax of the preamble following a
   * "CREATE TABLE" SQL statement.
   * NB: by default, this method returns an empty string.
   * It must be overwritten for those SQL backends which allow such
   * preambles such as, e.g., MySQL.
   */
  virtual svtkStdString GetTablePreamble(bool) { return svtkStdString(); }

  /**
   * Return the SQL string with the syntax to create a column inside a
   * "CREATE TABLE" SQL statement.
   * NB: this method implements the following minimally-portable syntax:
   * <column name> <column type> <column attributes>
   * It must be overwritten for those SQL backends which have a different
   * syntax such as, e.g., MySQL.
   */
  virtual svtkStdString GetColumnSpecification(
    svtkSQLDatabaseSchema* schema, int tblHandle, int colHandle);

  /**
   * Return the SQL string with the syntax to create an index inside a
   * "CREATE TABLE" SQL statement.
   * NB1: this method implements the following minimally-portable syntax:
   * <index type> [<index name>] (<column name 1>,... )
   * It must be overwritten for those SQL backends which have a different
   * syntax such as, e.g., MySQL.
   * NB2: this method does not assume that INDEX creation is supported
   * within a CREATE TABLE statement. Therefore, should such an INDEX arise
   * in the schema, a CREATE INDEX statement is returned and skipped is
   * set to true. Otherwise, skipped will always be returned false.
   */
  virtual svtkStdString GetIndexSpecification(
    svtkSQLDatabaseSchema* schema, int tblHandle, int idxHandle, bool& skipped);

  /**
   * Return the SQL string with the syntax to create a trigger using a
   * "CREATE TRIGGER" SQL statement.
   * NB1: support is contingent on SVTK_FEATURE_TRIGGERS being recognized as
   * a supported feature. Not all backends (e.g., SQLite) support it.
   * NB2: this method implements the following minimally-portable syntax:
   * <trigger name> {BEFORE | AFTER} <event> ON <table name> FOR EACH ROW <trigger action>
   * It must be overwritten for those SQL backends which have a different
   * syntax such as, e.g., PostgreSQL.
   */
  virtual svtkStdString GetTriggerSpecification(
    svtkSQLDatabaseSchema* schema, int tblHandle, int trgHandle);

  /**
   * Create a the proper subclass given a URL.
   * The URL format for SQL databases is a true URL of the form:
   * 'protocol://'[[username[':'password]'@']hostname[':'port]]'/'[dbname] .
   */
  static SVTK_NEWINSTANCE svtkSQLDatabase* CreateFromURL(const char* URL);

  /**
   * Effect a database schema.
   */
  virtual bool EffectSchema(svtkSQLDatabaseSchema*, bool dropIfExists = false);

  /**
   * Type for CreateFromURL callback.
   */
  typedef svtkSQLDatabase* (*CreateFunction)(const char* URL);

  //@{
  /**
   * Provides mechanism to register/unregister additional callbacks to create
   * concrete subclasses of svtkSQLDatabase to handle different protocols.
   * The registered callbacks are tried in the order they are registered.
   */
  static void RegisterCreateFromURLCallback(CreateFunction callback);
  static void UnRegisterCreateFromURLCallback(CreateFunction callback);
  static void UnRegisterAllCreateFromURLCallbacks();
  //@}

  /**
   * Stores the database class pointer as an information key. This is currently
   * used to store database pointers as part of 'data on demand' data objects.
   * For example: The application may have a table/tree/whatever of documents,
   * the data structure is storing the meta-data but not the full text. Further
   * down the pipeline algorithms or views may want to retrieve additional
   * information (full text)for specific documents.
   */
  static svtkInformationObjectBaseKey* DATABASE();

protected:
  svtkSQLDatabase();
  ~svtkSQLDatabase() override;

  /**
   * Subclasses should override this method to determine connection parameters
   * given the URL. This is called by CreateFromURL() to initialize the instance.
   * Look at CreateFromURL() for details about the URL format.
   */
  virtual bool ParseURL(const char* url) = 0;

private:
  svtkSQLDatabase(const svtkSQLDatabase&) = delete;
  void operator=(const svtkSQLDatabase&) = delete;

  //@{
  /**
   * Datastructure used to store registered callbacks.
   */
  class svtkCallbackVector;
  static svtkCallbackVector* Callbacks;
  //@}
};

#endif // svtkSQLDatabase_h
