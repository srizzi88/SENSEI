/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMySQLDatabase.h

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
 * @class   svtkMySQLDatabase
 * @brief   maintain a connection to a MySQL database
 *
 *
 *
 * This class provides a SVTK interface to MySQL
 * (http://www.mysql.com).  Unlike file-based databases like SQLite, you
 * talk to MySQL through a client/server connection.  You must specify
 * the hostname, (optional) port to connect to, username, password and
 * database name in order to connect.
 *
 * @sa
 * svtkMySQLQuery
 */

#ifndef svtkMySQLDatabase_h
#define svtkMySQLDatabase_h

#include "svtkIOMySQLModule.h" // For export macro
#include "svtkSQLDatabase.h"

class svtkSQLQuery;
class svtkMySQLQuery;
class svtkStringArray;
class svtkMySQLDatabasePrivate;

class SVTKIOMYSQL_EXPORT svtkMySQLDatabase : public svtkSQLDatabase
{

  friend class svtkMySQLQuery;

public:
  svtkTypeMacro(svtkMySQLDatabase, svtkSQLDatabase);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkMySQLDatabase* New();

  /**
   * Open a new connection to the database.  You need to set the
   * filename before calling this function.  Returns true if the
   * database was opened successfully; false otherwise.
   */
  bool Open(const char* password = 0) override;

  /**
   * Close the connection to the database.
   */
  void Close() override;

  /**
   * Return whether the database has an open connection
   */
  bool IsOpen() override;

  /**
   * Return an empty query on this database.
   */
  svtkSQLQuery* GetQueryInstance() override;

  /**
   * Get the list of tables from the database
   */
  svtkStringArray* GetTables() override;

  /**
   * Get the list of fields for a particular table
   */
  svtkStringArray* GetRecord(const char* table) override;

  /**
   * Return whether a feature is supported by the database.
   */
  bool IsSupported(int feature) override;

  /**
   * Did the last operation generate an error
   */
  bool HasError() override;

  /**
   * Get the last error text from the database
   */
  const char* GetLastErrorText() override;

  //@{
  /**
   * String representing database type (e.g. "mysql").
   */
  const char* GetDatabaseType() override { return this->DatabaseType; }
  //@}

  //@{
  /**
   * The database server host name.
   */
  svtkSetStringMacro(HostName);
  svtkGetStringMacro(HostName);
  //@}

  //@{
  /**
   * The user name for connecting to the database server.
   */
  svtkSetStringMacro(User);
  svtkGetStringMacro(User);
  //@}

  //@{
  /**
   * The user's password for connecting to the database server.
   */
  svtkSetStringMacro(Password);
  //@}

  //@{
  /**
   * The name of the database to connect to.
   */
  svtkSetStringMacro(DatabaseName);
  svtkGetStringMacro(DatabaseName);
  //@}

  //@{
  /**
   * Should automatic reconnection be enabled?
   * This defaults to true.
   * If you change its value, you must do so before any call to Open().
   */
  svtkSetMacro(Reconnect, int);
  svtkGetMacro(Reconnect, int);
  svtkBooleanMacro(Reconnect, int);
  //@}

  //@{
  /**
   * The port used for connecting to the database.
   */
  svtkSetClampMacro(ServerPort, int, 0, SVTK_INT_MAX);
  svtkGetMacro(ServerPort, int);
  //@}

  /**
   * Get the URL of the database.
   */
  svtkStdString GetURL() override;

  /**
   * Return the SQL string with the syntax of the preamble following a
   * "CREATE TABLE" SQL statement.
   * NB: this method implements the MySQL-specific IF NOT EXISTS syntax,
   * used when b = false.
   */
  svtkStdString GetTablePreamble(bool b) override { return b ? svtkStdString() : "IF NOT EXISTS "; }

  /**
   * Return the SQL string with the syntax to create a column inside a
   * "CREATE TABLE" SQL statement.
   * NB1: this method implements the MySQL-specific syntax:
   * \verbatim
   * `<column name>` <column type> <column attributes>
   * \endverbatim
   * NB2: if a column has type SERIAL in the schema, this will be turned
   * into INT NOT nullptr AUTO_INCREMENT. Therefore, one should not pass
   * NOT nullptr as an attribute of a column whose type is SERIAL.
   */
  svtkStdString GetColumnSpecification(
    svtkSQLDatabaseSchema* schema, int tblHandle, int colHandle) override;

  /**
   * Return the SQL string with the syntax to create an index inside a
   * "CREATE TABLE" SQL statement.
   * NB1: this method implements the MySQL-specific syntax:
   * \verbatim
   * <index type> [<index name>]  (`<column name 1>`,... )
   * \endverbatim
   * NB2: since MySQL supports INDEX creation within a CREATE TABLE statement,
   * skipped is always returned false.
   */
  svtkStdString GetIndexSpecification(
    svtkSQLDatabaseSchema* schema, int tblHandle, int idxHandle, bool& skipped) override;

  /**
   * Create a new database, optionally dropping any existing database of the same name.
   * Returns true when the database is properly created and false on failure.
   */
  bool CreateDatabase(const char* dbName, bool dropExisting);

  /**
   * Drop a database if it exists.
   * Returns true on success and false on failure.
   */
  bool DropDatabase(const char* dbName);

  /**
   * Overridden to determine connection parameters given the URL.
   * This is called by CreateFromURL() to initialize the instance.
   * Look at CreateFromURL() for details about the URL format.
   */
  bool ParseURL(const char* url) override;

protected:
  svtkMySQLDatabase();
  ~svtkMySQLDatabase() override;

private:
  // We want this to be private, a user of this class
  // should not be setting this for any reason
  svtkSetStringMacro(DatabaseType);

  svtkStringArray* Tables;
  svtkStringArray* Record;

  char* DatabaseType;
  char* HostName;
  char* User;
  char* Password;
  char* DatabaseName;
  int ServerPort;
  int Reconnect;

  svtkMySQLDatabasePrivate* const Private;

  svtkMySQLDatabase(const svtkMySQLDatabase&) = delete;
  void operator=(const svtkMySQLDatabase&) = delete;
};

#endif // svtkMySQLDatabase_h
