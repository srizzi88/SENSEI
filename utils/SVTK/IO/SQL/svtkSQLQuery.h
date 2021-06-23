/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLQuery.h

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
 * @class   svtkSQLQuery
 * @brief   executes an sql query and retrieves results
 *
 *
 * The abstract superclass of SQL query classes.  Instances of subclasses
 * of svtkSQLQuery are created using the GetQueryInstance() function in
 * svtkSQLDatabase.  To implement a query connection for a new database
 * type, subclass both svtkSQLDatabase and svtkSQLQuery, and implement the
 * required functions.  For the query class, this involves the following:
 *
 * Execute() - Execute the query on the database.  No results need to be
 *             retrieved at this point, unless you are performing caching.
 *
 * GetNumberOfFields() - After Execute() is performed, returns the number
 *                       of fields in the query results.
 *
 * GetFieldName() - The name of the field at an index.
 *
 * GetFieldType() - The data type of the field at an index.
 *
 * NextRow() - Advances the query results by one row, and returns whether
 *             there are more rows left in the query.
 *
 * DataValue() - Extract a single data value from the current row.
 *
 * Begin/Rollback/CommitTransaction() - These methods are optional but
 * recommended if the database supports transactions.
 *
 * @par Thanks:
 * Thanks to Andrew Wilson from Sandia National Laboratories for his work
 * on the database classes.
 *
 * @sa
 * svtkSQLDatabase
 */

#ifndef svtkSQLQuery_h
#define svtkSQLQuery_h

#include "svtkIOSQLModule.h" // For export macro
#include "svtkRowQuery.h"
#include "svtkStdString.h" // for EscapeString()

class svtkSQLDatabase;
class svtkVariant;
class svtkVariantArray;

class SVTKIOSQL_EXPORT svtkSQLQuery : public svtkRowQuery
{
public:
  svtkTypeMacro(svtkSQLQuery, svtkRowQuery);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The query string to be executed.  Since some databases will
   * process the query string as soon as it's set, this method returns
   * a boolean to indicate success or failure.
   */
  virtual bool SetQuery(const char* query);
  virtual const char* GetQuery();
  //@}

  /**
   * Return true if the query is active (i.e. execution was successful
   * and results are ready to be fetched).  Returns false on error or
   * inactive query.
   */
  bool IsActive() override { return this->Active; }

  /**
   * Execute the query.  This must be performed
   * before any field name or data access functions
   * are used.
   */
  bool Execute() override = 0;

  /**
   * Begin, commit, or roll back a transaction.  If the underlying
   * database does not support transactions these calls will do
   * nothing.
   */
  virtual bool BeginTransaction() { return true; }
  virtual bool CommitTransaction() { return true; }
  virtual bool RollbackTransaction() { return true; }

  //@{
  /**
   * Return the database associated with the query.
   */
  svtkGetObjectMacro(Database, svtkSQLDatabase);
  //@}

  /**
   * Bind a parameter to a placeholder in a query.  A full discussion
   * of this feature is beyond the scope of this header file, but in
   * short, here's how it works:

   * Instead of saying "SELECT foo FROM mytable WHERE myfield = 12345"
   * you can say "SELECT foo FROM mytable WHERE myfield = ?".  The ?
   * character is a placeholder for a parameter that must then be
   * bound.  Call BindParameter(0, 12345) to bind the integer value
   * 12345 to that field.  Placeholders are indexed starting at 0.

   * You are responsible for making sure that the types match when you
   * call BindParameter.  You don't have to get it precisely correct:
   * in general, the SQL driver is smart enough to do things like cast
   * a short to a long or a float to a double.

   * Bound parameters were introduced in ANSI SQL 92.  Please see that
   * standard for more information.

   * Most of these methods are excluded from wrapping because the Java
   * wrapper treats all integer types from char up through 64-bit long
   * as 'int'.  This is incorrect behavior but what else am I going to
   * do?

   * Finally, the default implementation for BindParameter(int,
   * svtkVariant) dispatches to one of the more type-specific versions.  It
   * should be OK to use in database drivers without modification.
   */

  virtual bool BindParameter(int index, unsigned char value);
  virtual bool BindParameter(int index, unsigned short value);
  virtual bool BindParameter(int index, unsigned int value);
  virtual bool BindParameter(int index, unsigned long value);
  // The C and C++ standards leave it up to each compiler to decide
  // whether chars are signed or unsigned by default.  All the other
  // types are signed unless otherwise specified.
  virtual bool BindParameter(int index, signed char value);
  virtual bool BindParameter(int index, short value);

  virtual bool BindParameter(int index, int value);

  virtual bool BindParameter(int index, long value);
  virtual bool BindParameter(int index, unsigned long long value);
  virtual bool BindParameter(int index, long long value);

  virtual bool BindParameter(int index, float value);
  virtual bool BindParameter(int index, double value);
  /**
   * Bind a string value -- string must be null-terminated
   */
  virtual bool BindParameter(int index, const char* stringValue);
  /**
   * Bind a string value by specifying an array and a size
   */
  virtual bool BindParameter(int index, const char* stringValue, size_t length);

  virtual bool BindParameter(int index, const svtkStdString& string);

  virtual bool BindParameter(int index, svtkVariant var);
  /**
   * Bind a blob value.  Not all databases support blobs as a data
   * type.  Check svtkSQLDatabase::IsSupported(SVTK_SQL_FEATURE_BLOB) to
   * make sure.
   */
  virtual bool BindParameter(int index, const void* data, size_t length);
  /**
   * Reset all parameter bindings to nullptr.
   */
  virtual bool ClearParameterBindings();

  /**
   * Escape a string for inclusion into an SQL query.
   * If \a addSurroundingQuotes is true, then quotation marks appropriate to the
   * backend database will be added to enclose the escaped string. This argument
   * defaults to true.

   * A default, simple-minded implementation is provided for
   * database backends that do not provde a way to escape
   * strings for use inside queries.
   */
  virtual svtkStdString EscapeString(svtkStdString s, bool addSurroundingQuotes = true);

  /**
   * Escape a string for inclusion into an SQL query.
   * This method exists to provide a wrappable version of
   * the method that takes and returns svtkStdString objects.
   * You are responsible for calling delete [] on the
   * character array returned by this method.
   * This method simply calls the svtkStdString variant and thus
   * need not be re-implemented by subclasses.
   */
  char* EscapeString(const char* src, bool addSurroundingQuotes);

protected:
  svtkSQLQuery();
  ~svtkSQLQuery() override;

  /**
   * Set the database associated with the query.
   * This is only to be called by the corresponding
   * database class on creation of the query in
   * GetQueryInstance().
   */
  void SetDatabase(svtkSQLDatabase* db);

  char* Query;
  svtkSQLDatabase* Database;
  bool Active;

private:
  svtkSQLQuery(const svtkSQLQuery&) = delete;
  void operator=(const svtkSQLQuery&) = delete;
};

#endif // svtkSQLQuery_h
