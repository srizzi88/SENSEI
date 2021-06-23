/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLiteDatabase.h

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
 * @class   svtkSQLiteDatabase
 * @brief   maintain a connection to an SQLite database
 *
 *
 *
 * SQLite (http://www.sqlite.org) is a public-domain SQL database
 * written in C++.  It's small, fast, and can be easily embedded
 * inside other applications.  Its databases are stored in files.
 *
 * This class provides a SVTK interface to SQLite.  You do not need to
 * download any external libraries: we include a copy of SQLite 3.3.16
 * in SVTK/Utilities/svtksqlite.
 *
 * If you want to open a database that stays in memory and never gets
 * written to disk, pass in the URL 'sqlite://:memory:'; otherwise,
 * specify the file path by passing the URL 'sqlite://<file_path>'.
 *
 * @par Thanks:
 * Thanks to Andrew Wilson and Philippe Pebay from Sandia National
 * Laboratories for implementing this class.
 *
 * @sa
 * svtkSQLiteQuery
 */

#ifndef svtkSQLiteDatabase_h
#define svtkSQLiteDatabase_h

#include "svtkIOSQLModule.h" // For export macro
#include "svtkSQLDatabase.h"

class svtkSQLQuery;
class svtkSQLiteQuery;
class svtkStringArray;
class svtkSQLiteDatabaseInternals;

class SVTKIOSQL_EXPORT svtkSQLiteDatabase : public svtkSQLDatabase
{

  friend class svtkSQLiteQuery;

public:
  svtkTypeMacro(svtkSQLiteDatabase, svtkSQLDatabase);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkSQLiteDatabase* New();

  enum
  {
    USE_EXISTING,
    USE_EXISTING_OR_CREATE,
    CREATE_OR_CLEAR,
    CREATE
  };

  //@{
  /**
   * Open a new connection to the database.  You need to set the
   * filename before calling this function.  Returns true if the
   * database was opened successfully; false otherwise.
   * - USE_EXISTING (default) - Fail if the file does not exist.
   * - USE_EXISTING_OR_CREATE - Create a new file if necessary.
   * - CREATE_OR_CLEAR - Create new or clear existing file.
   * - CREATE - Create new, fail if file exists.
   */
  bool Open(const char* password) override;
  bool Open(const char* password, int mode);
  //@}

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
   * String representing database type (e.g. "sqlite").
   */
  const char* GetDatabaseType() override { return this->DatabaseType; }
  //@}

  //@{
  /**
   * String representing the database filename.
   */
  svtkGetStringMacro(DatabaseFileName);
  svtkSetStringMacro(DatabaseFileName);
  //@}

  /**
   * Get the URL of the database.
   */
  svtkStdString GetURL() override;

  /**
   * Return the SQL string with the syntax to create a column inside a
   * "CREATE TABLE" SQL statement.
   * NB: this method implements the SQLite-specific syntax:
   * <column name> <column type> <column attributes>
   */
  svtkStdString GetColumnSpecification(
    svtkSQLDatabaseSchema* schema, int tblHandle, int colHandle) override;

protected:
  svtkSQLiteDatabase();
  ~svtkSQLiteDatabase() override;

  /**
   * Overridden to determine connection parameters given the URL.
   * This is called by CreateFromURL() to initialize the instance.
   * Look at CreateFromURL() for details about the URL format.
   */
  bool ParseURL(const char* url) override;

private:
  svtkSQLiteDatabaseInternals* Internal;

  // We want this to be private, a user of this class
  // should not be setting this for any reason
  svtkSetStringMacro(DatabaseType);

  svtkStringArray* Tables;

  char* DatabaseType;
  char* DatabaseFileName;

  svtkStdString TempURL;

  svtkSQLiteDatabase(const svtkSQLiteDatabase&) = delete;
  void operator=(const svtkSQLiteDatabase&) = delete;
};

#endif // svtkSQLiteDatabase_h
