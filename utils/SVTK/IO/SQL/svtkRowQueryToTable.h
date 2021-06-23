/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRowQueryToTable.h

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
 * @class   svtkRowQueryToTable
 * @brief   executes an sql query and retrieves results into a table
 *
 *
 * svtkRowQueryToTable creates a svtkTable with the results of an arbitrary SQL
 * query.  To use this filter, you first need an instance of a svtkSQLDatabase
 * subclass.  You may use the database class to obtain a svtkRowQuery instance.
 * Set that query on this filter to extract the query as a table.
 *
 * @par Thanks:
 * Thanks to Andrew Wilson from Sandia National Laboratories for his work
 * on the database classes.
 *
 * @sa
 * svtkSQLDatabase svtkRowQuery
 */

#ifndef svtkRowQueryToTable_h
#define svtkRowQueryToTable_h

#include "svtkIOSQLModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class svtkRowQuery;

class SVTKIOSQL_EXPORT svtkRowQueryToTable : public svtkTableAlgorithm
{
public:
  static svtkRowQueryToTable* New();
  svtkTypeMacro(svtkRowQueryToTable, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The query to execute.
   */
  void SetQuery(svtkRowQuery* query);
  svtkGetObjectMacro(Query, svtkRowQuery);
  //@}

  /**
   * Update the modified time based on the query.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkRowQueryToTable();
  ~svtkRowQueryToTable() override;

  svtkRowQuery* Query;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkRowQueryToTable(const svtkRowQueryToTable&) = delete;
  void operator=(const svtkRowQueryToTable&) = delete;
};

#endif
