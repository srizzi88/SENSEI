/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtSQLQuery.h

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
 * @class   svtkQtSQLQuery
 * @brief   query class associated with svtkQtSQLDatabase
 *
 *
 * Implements svtkSQLQuery using an underlying QSQLQuery.
 */

#ifndef svtkQtSQLQuery_h
#define svtkQtSQLQuery_h

// Check for Qt SQL module before defining this class.
#include <qglobal.h> // Needed to check if SQL is available
#if (QT_EDITION & QT_MODULE_SQL)

#include "svtkGUISupportQtSQLModule.h" // For export macro
#include "svtkSQLQuery.h"

class svtkVariant;
class svtkQtSQLQueryInternals;

class SVTKGUISUPPORTQTSQL_EXPORT svtkQtSQLQuery : public svtkSQLQuery
{
public:
  static svtkQtSQLQuery* New();
  svtkTypeMacro(svtkQtSQLQuery, svtkSQLQuery);
  void PrintSelf(ostream& os, svtkIndent indent) override;

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
  const char* GetFieldName(int col) override;

  /**
   * Return the type of the specified query field, as defined in svtkType.h.
   */
  int GetFieldType(int col) override;

  /**
   * Advance row, return false if past end.
   */
  bool NextRow() override;

  /**
   * Return data in current row, field c
   */
  svtkVariant DataValue(svtkIdType c) override;

  /**
   * Returns true if an error is set, otherwise false.
   */
  bool HasError() override;

  /**
   * Get the last error text from the query
   */
  const char* GetLastErrorText() override;

protected:
  svtkQtSQLQuery();
  ~svtkQtSQLQuery() override;

  svtkQtSQLQueryInternals* Internals;
  friend class svtkQtSQLDatabase;

private:
  // Using the convenience function internally
  svtkSetStringMacro(LastErrorText);

  char* LastErrorText;

  svtkQtSQLQuery(const svtkQtSQLQuery&) = delete;
  void operator=(const svtkQtSQLQuery&) = delete;
};

#endif // (QT_EDITION & QT_MODULE_SQL)
#endif // svtkQtSQLQuery_h
