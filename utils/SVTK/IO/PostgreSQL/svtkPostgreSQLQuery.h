/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPostgreSQLQuery.h

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
 * @class   svtkPostgreSQLQuery
 * @brief   svtkSQLQuery implementation for PostgreSQL databases
 *
 *
 *
 * This is an implementation of svtkSQLQuery for PostgreSQL databases.  See
 * the documentation for svtkSQLQuery for information about what the
 * methods do.
 *
 *
 * @par Thanks:
 * Thanks to David Thompson and Andy Wilson from Sandia National
 * Laboratories for implementing this class.
 *
 * @sa
 * svtkSQLDatabase svtkSQLQuery svtkPostgreSQLDatabase
 */

#ifndef svtkPostgreSQLQuery_h
#define svtkPostgreSQLQuery_h

#include "svtkIOPostgreSQLModule.h" // For export macro
#include "svtkSQLQuery.h"

class svtkPostgreSQLDatabase;
class svtkVariant;
class svtkVariantArray;
class svtkPostgreSQLQueryPrivate;

class SVTKIOPOSTGRESQL_EXPORT svtkPostgreSQLQuery : public svtkSQLQuery
{
public:
  static svtkPostgreSQLQuery* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkPostgreSQLQuery, svtkSQLQuery);

  /**
   * Execute the query.  This must be performed
   * before any field name or data access functions
   * are used.
   */
  bool Execute() override;

  /**
   * The number of fields in the query result.
   */
  int GetNumberOfFields() override;

  /**
   * Return the name of the specified query field.
   */
  const char* GetFieldName(int i) override;

  /**
   * Return the type of the field, using the constants defined in svtkType.h.
   */
  int GetFieldType(int i) override;

  /**
   * Advance row, return false if past end.
   */
  bool NextRow() override;

  /**
   * Return true if there is an error on the current query.
   */
  bool HasError() override;

  //@{
  /**
   * Begin, abort (roll back), or commit a transaction.
   */
  bool BeginTransaction() override;
  bool RollbackTransaction() override;
  bool CommitTransaction() override;
  //@}

  /**
   * Return data in current row, field c
   */
  svtkVariant DataValue(svtkIdType c) override;

  /**
   * Get the last error text from the query
   */
  const char* GetLastErrorText() override;

  /**
   * Escape a string for inclusion into an SQL query
   */
  svtkStdString EscapeString(svtkStdString s, bool addSurroundingQuotes = true) override;

  /**
   * Unlike some databases, Postgres can tell you right away how many
   * rows are in the results of your query.
   */
  int GetNumberOfRows();

protected:
  svtkPostgreSQLQuery();
  ~svtkPostgreSQLQuery() override;

  svtkSetStringMacro(LastErrorText);

  bool IsColumnBinary(int whichColumn);
  const char* GetColumnRawData(int whichColumn);

  bool TransactionInProgress;
  char* LastErrorText;
  int CurrentRow;

  svtkPostgreSQLQueryPrivate* QueryInternals;

  void DeleteQueryResults();

  friend class svtkPostgreSQLDatabase;

private:
  svtkPostgreSQLQuery(const svtkPostgreSQLQuery&) = delete;
  void operator=(const svtkPostgreSQLQuery&) = delete;
};

#endif // svtkPostgreSQLQuery_h
