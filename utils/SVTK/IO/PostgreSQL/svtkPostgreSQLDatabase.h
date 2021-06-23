/* -*- Mode: C++; -*- */
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPostgreSQLDatabase.h

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
 * @class   svtkPostgreSQLDatabase
 * @brief   maintain a connection to a PostgreSQL database
 *
 *
 *
 * PostgreSQL (http://www.postgres.org) is a BSD-licensed SQL database.
 * It's large, fast, and can not be easily embedded
 * inside other applications.  Its databases are stored in files that
 * belong to another process.
 *
 * This class provides a SVTK interface to PostgreSQL.  You do need to
 * download external libraries: we need a copy of PostgreSQL 8
 * (currently 8.2 or 8.3) so that we can link against the libpq C
 * interface.
 *
 *
 * @par Thanks:
 * Thanks to David Thompson and Andy Wilson from Sandia National
 * Laboratories for implementing this class.
 *
 * @sa
 * svtkPostgreSQLQuery
 */

#ifndef svtkPostgreSQLDatabase_h
#define svtkPostgreSQLDatabase_h

#include "svtkIOPostgreSQLModule.h" // For export macro
#include "svtkSQLDatabase.h"

class svtkPostgreSQLQuery;
class svtkStringArray;
class svtkPostgreSQLDatabasePrivate;
struct PQconn;

class SVTKIOPOSTGRESQL_EXPORT svtkPostgreSQLDatabase : public svtkSQLDatabase
{

  friend class svtkPostgreSQLQuery;
  friend class svtkPostgreSQLQueryPrivate;

public:
  svtkTypeMacro(svtkPostgreSQLDatabase, svtkSQLDatabase);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkPostgreSQLDatabase* New();

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
   * Did the last operation generate an error
   */
  virtual bool HasError() override;

  /**
   * Get the last error text from the database
   */
  const char* GetLastErrorText() override;

  //@{
  /**
   * String representing database type (e.g. "psql").
   */
  const char* GetDatabaseType() override { return this->DatabaseType; }
  //@}

  //@{
  /**
   * The database server host name.
   */
  virtual void SetHostName(const char*);
  svtkGetStringMacro(HostName);
  //@}

  //@{
  /**
   * The user name for connecting to the database server.
   */
  virtual void SetUser(const char*);
  svtkGetStringMacro(User);
  //@}

  /**
   * The user's password for connecting to the database server.
   */
  virtual void SetPassword(const char*);

  //@{
  /**
   * The name of the database to connect to.
   */
  virtual void SetDatabaseName(const char*);
  svtkGetStringMacro(DatabaseName);
  //@}

  //@{
  /**
   * Additional options for the database.
   */
  virtual void SetConnectOptions(const char*);
  svtkGetStringMacro(ConnectOptions);
  //@}

  //@{
  /**
   * The port used for connecting to the database.
   */
  virtual void SetServerPort(int);
  virtual int GetServerPortMinValue() { return 0; }
  virtual int GetServerPortMaxValue() { return SVTK_INT_MAX; }
  svtkGetMacro(ServerPort, int);
  //@}

  /**
   * Get a URL referencing the current database connection.
   * This is not well-defined if the HostName and DatabaseName
   * have not been set. The URL will be of the form
   * <code>'psql://'[username[':'password]'@']hostname[':'port]'/'database</code> .
   */
  svtkStdString GetURL() override;

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
   * Return a list of databases on the server.
   */
  svtkStringArray* GetDatabases();

  /**
   * Create a new database, optionally dropping any existing database of the same name.
   * Returns true when the database is properly created and false on failure.
   */
  bool CreateDatabase(const char* dbName, bool dropExisting = false);

  /**
   * Drop a database if it exists.
   * Returns true on success and false on failure.
   */
  bool DropDatabase(const char* dbName);

  /**
   * Return the SQL string with the syntax to create a column inside a
   * "CREATE TABLE" SQL statement.
   * NB: this method implements the PostgreSQL-specific syntax:
   * <column name> <column type> <column attributes>
   */
  svtkStdString GetColumnSpecification(
    svtkSQLDatabaseSchema* schema, int tblHandle, int colHandle) override;

  /**
   * Overridden to determine connection parameters given the URL.
   * This is called by CreateFromURL() to initialize the instance.
   * Look at CreateFromURL() for details about the URL format.
   */
  bool ParseURL(const char* url) override;

protected:
  svtkPostgreSQLDatabase();
  ~svtkPostgreSQLDatabase() override;

  /**
   * Create or refresh the map from Postgres column types to SVTK array types.

   * Postgres defines a table for types so that users may define types.
   * This adaptor does not support user-defined types or even all of the
   * default types defined by Postgres (some are inherently difficult to
   * translate into SVTK since Postgres allows columns to have composite types,
   * vector-valued types, and extended precision types that svtkVariant does
   * not support.

   * This routine examines the pg_types table to get a map from Postgres column
   * type IDs (stored as OIDs) to SVTK array types. It is called whenever a new
   * database connection is initiated.
   */
  void UpdateDataTypeMap();

  svtkSetStringMacro(DatabaseType);
  svtkSetStringMacro(LastErrorText);
  void NullTrailingWhitespace(char* msg);
  bool OpenInternal(const char* connectionOptions);

  svtkTimeStamp URLMTime;
  svtkPostgreSQLDatabasePrivate* Connection;
  svtkTimeStamp ConnectionMTime;
  svtkStringArray* Tables;
  char* DatabaseType;
  char* HostName;
  char* User;
  char* Password;
  char* DatabaseName;
  int ServerPort;
  char* ConnectOptions;
  char* LastErrorText;

private:
  svtkPostgreSQLDatabase(const svtkPostgreSQLDatabase&) = delete;
  void operator=(const svtkPostgreSQLDatabase&) = delete;
};

// This is basically the body of the SetStringMacro but with a
// call to update an additional svtkTimeStamp. We inline the implementation
// so that wrapping will work.
#define svtkSetStringPlusMTimeMacro(className, name, timeStamp)                                     \
  inline void className::Set##name(const char* _arg)                                               \
  {                                                                                                \
    svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting " << #name " to "         \
                  << (_arg ? _arg : "(null)"));                                                    \
    if (this->name == nullptr && _arg == nullptr)                                                  \
    {                                                                                              \
      return;                                                                                      \
    }                                                                                              \
    if (this->name && _arg && (!strcmp(this->name, _arg)))                                         \
    {                                                                                              \
      return;                                                                                      \
    }                                                                                              \
    delete[] this->name;                                                                           \
    if (_arg)                                                                                      \
    {                                                                                              \
      size_t n = strlen(_arg) + 1;                                                                 \
      char* cp1 = new char[n];                                                                     \
      const char* cp2 = (_arg);                                                                    \
      this->name = cp1;                                                                            \
      do                                                                                           \
      {                                                                                            \
        *cp1++ = *cp2++;                                                                           \
      } while (--n);                                                                               \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      this->name = nullptr;                                                                        \
    }                                                                                              \
    this->Modified();                                                                              \
    this->timeStamp.Modified();                                                                    \
    this->Close(); /* Force a re-open on next query */                                             \
  }

svtkSetStringPlusMTimeMacro(svtkPostgreSQLDatabase, HostName, URLMTime);
svtkSetStringPlusMTimeMacro(svtkPostgreSQLDatabase, User, URLMTime);
svtkSetStringPlusMTimeMacro(svtkPostgreSQLDatabase, Password, URLMTime);
svtkSetStringPlusMTimeMacro(svtkPostgreSQLDatabase, DatabaseName, URLMTime);
svtkSetStringPlusMTimeMacro(svtkPostgreSQLDatabase, ConnectOptions, URLMTime);

inline void svtkPostgreSQLDatabase::SetServerPort(int _arg)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting ServerPort to " << _arg);
  if (this->ServerPort != (_arg < 0 ? 0 : (_arg > SVTK_INT_MAX ? SVTK_INT_MAX : _arg)))
  {
    this->ServerPort = (_arg < 0 ? 0 : (_arg > SVTK_INT_MAX ? SVTK_INT_MAX : _arg));
    this->Modified();
    this->URLMTime.Modified();
    this->Close(); // Force a re-open on next query
  }
}

#endif // svtkPostgreSQLDatabase_h
