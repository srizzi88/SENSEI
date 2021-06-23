/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLiteQuery.h

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
 * @class   svtkSQLiteQuery
 * @brief   svtkSQLQuery implementation for SQLite databases
 *
 *
 *
 * This is an implementation of svtkSQLQuery for SQLite databases.  See
 * the documentation for svtkSQLQuery for information about what the
 * methods do.
 *
 *
 * @bug
 * Sometimes Execute() will return false (meaning an error) but
 * GetLastErrorText() winds up null.  I am not certain why this is
 * happening.
 *
 * @par Thanks:
 * Thanks to Andrew Wilson from Sandia National Laboratories for implementing
 * this class.
 *
 * @sa
 * svtkSQLDatabase svtkSQLQuery svtkSQLiteDatabase
 */

#ifndef svtkSQLiteQuery_h
#define svtkSQLiteQuery_h

#include "svtkIOSQLModule.h" // For export macro
#include "svtkSQLQuery.h"

class svtkSQLiteDatabase;
class svtkVariant;
class svtkVariantArray;

class SVTKIOSQL_EXPORT svtkSQLiteQuery : public svtkSQLQuery
{

  friend class svtkSQLiteDatabase;

public:
  svtkTypeMacro(svtkSQLiteQuery, svtkSQLQuery);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkSQLiteQuery* New();

  /**
   * Set the SQL query string.  This must be performed before
   * Execute() or BindParameter() can be called.
   */
  bool SetQuery(const char* query) override;

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
   * The following methods bind a parameter value to a placeholder in
   * the SQL string.  See the documentation for svtkSQLQuery for
   * further explanation.  The driver makes internal copies of string
   * and BLOB parameters so you don't need to worry about keeping them
   * in scope until the query finishes executing.
   */

  using svtkSQLQuery::BindParameter;
  bool BindParameter(int index, unsigned char value) override;
  bool BindParameter(int index, signed char value) override;
  bool BindParameter(int index, unsigned short value) override;
  bool BindParameter(int index, short value) override;
  bool BindParameter(int index, unsigned int value) override;

  bool BindParameter(int index, int value) override;

  bool BindParameter(int index, unsigned long value) override;
  bool BindParameter(int index, long value) override;
  bool BindParameter(int index, unsigned long long value) override;
  bool BindParameter(int index, long long value) override;

  bool BindParameter(int index, float value) override;
  bool BindParameter(int index, double value) override;
  /**
   * Bind a string value -- string must be null-terminated
   */
  bool BindParameter(int index, const char* stringValue) override;
  /**
   * Bind a string value by specifying an array and a size
   */
  bool BindParameter(int index, const char* stringValue, size_t length) override;

  bool BindParameter(int index, const svtkStdString& string) override;

  bool BindParameter(int index, svtkVariant value) override;
  //@{
  /**
   * Bind a blob value.  Not all databases support blobs as a data
   * type.  Check svtkSQLDatabase::IsSupported(SVTK_SQL_FEATURE_BLOB) to
   * make sure.
   */
  bool BindParameter(int index, const void* data, size_t length) override;
  bool ClearParameterBindings() override;
  //@}

protected:
  svtkSQLiteQuery();
  ~svtkSQLiteQuery() override;

  svtkSetStringMacro(LastErrorText);

private:
  svtkSQLiteQuery(const svtkSQLiteQuery&) = delete;
  void operator=(const svtkSQLiteQuery&) = delete;

  class Priv;
  Priv* Private;
  bool InitialFetch;
  int InitialFetchResult;
  char* LastErrorText;
  bool TransactionInProgress;

  //@{
  /**
   * All of the BindParameter calls fall through to these methods
   * where we actually talk to sqlite.  You don't need to call them directly.
   */
  bool BindIntegerParameter(int index, int value);
  bool BindDoubleParameter(int index, double value);
  bool BindInt64Parameter(int index, svtkTypeInt64 value);
  bool BindStringParameter(int index, const char* data, int length);
  bool BindBlobParameter(int index, const void* data, int length);
  //@}
};

#endif // svtkSQLiteQuery_h
