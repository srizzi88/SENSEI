/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMySQLQuery.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMySQLQuery
 * @brief   svtkSQLQuery implementation for MySQL databases
 *
 *
 *
 * This is an implementation of svtkSQLQuery for MySQL databases.  See
 * the documentation for svtkSQLQuery for information about what the
 * methods do.
 *
 *
 * @bug
 * Since MySQL requires that all bound parameters be passed in a
 * single mysql_stmt_bind_param call, there is no way to determine
 * which one is causing an error when one occurs.
 *
 *
 * @sa
 * svtkSQLDatabase svtkSQLQuery svtkMySQLDatabase
 */

#ifndef svtkMySQLQuery_h
#define svtkMySQLQuery_h

#include "svtkIOMySQLModule.h" // For export macro
#include "svtkSQLQuery.h"

class svtkMySQLDatabase;
class svtkVariant;
class svtkVariantArray;
class svtkMySQLQueryInternals;

class SVTKIOMYSQL_EXPORT svtkMySQLQuery : public svtkSQLQuery
{

  friend class svtkMySQLDatabase;

public:
  svtkTypeMacro(svtkMySQLQuery, svtkSQLQuery);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkMySQLQuery* New();

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

  //@{
  /**
   * Begin, commit, or roll back a transaction.

   * Calling any of these methods will overwrite the current query text
   * and call Execute() so any previous query text and results will be lost.
   */
  bool BeginTransaction() override;
  bool CommitTransaction() override;
  bool RollbackTransaction() override;
  //@}

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
  bool BindParameter(int index, signed short value) override;
  bool BindParameter(int index, unsigned int value) override;

  bool BindParameter(int index, int value) override;

  bool BindParameter(int index, unsigned long value) override;
  bool BindParameter(int index, signed long value) override;
  bool BindParameter(int index, unsigned long long value) override;
  bool BindParameter(int index, long long value) override;

  bool BindParameter(int index, float value) override;
  bool BindParameter(int index, double value) override;
  /**
   * Bind a string value -- string must be null-terminated
   */
  bool BindParameter(int index, const char* stringValue) override;
  //@{
  /**
   * Bind a string value by specifying an array and a size
   */
  bool BindParameter(int index, const char* stringValue, size_t length) override;
  bool BindParameter(int index, const svtkStdString& string) override;
  //@}

  //@{
  /**
   * Bind a blob value.  Not all databases support blobs as a data
   * type.  Check svtkSQLDatabase::IsSupported(SVTK_SQL_FEATURE_BLOB) to
   * make sure.
   */
  bool BindParameter(int index, const void* data, size_t length) override;
  bool ClearParameterBindings() override;
  //@}

  /**
   * Escape a string for use in a query
   */
  svtkStdString EscapeString(svtkStdString src, bool addSurroundingQuotes = true) override;

protected:
  svtkMySQLQuery();
  ~svtkMySQLQuery() override;

  svtkSetStringMacro(LastErrorText);

private:
  svtkMySQLQuery(const svtkMySQLQuery&) = delete;
  void operator=(const svtkMySQLQuery&) = delete;

  svtkMySQLQueryInternals* Internals;
  bool InitialFetch;
  char* LastErrorText;
};

#endif // svtkMySQLQuery_h
