/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSQLDatabaseTableSource.h

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
 * @class   svtkSQLDatabaseTableSource
 * @brief   Generates a svtkTable based on an SQL query.
 *
 *
 * This class combines svtkSQLDatabase, svtkSQLQuery, and svtkQueryToTable to
 * provide a convenience class for generating tables from databases.
 * Also this class can be easily wrapped and used within ParaView / OverView.
 */

#ifndef svtkSQLDatabaseTableSource_h
#define svtkSQLDatabaseTableSource_h

#include "svtkIOSQLModule.h" // For export macro
#include "svtkStdString.h"
#include "svtkTableAlgorithm.h"

class svtkEventForwarderCommand;

class SVTKIOSQL_EXPORT svtkSQLDatabaseTableSource : public svtkTableAlgorithm
{
public:
  static svtkSQLDatabaseTableSource* New();
  svtkTypeMacro(svtkSQLDatabaseTableSource, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkStdString GetURL();
  void SetURL(const svtkStdString& url);

  void SetPassword(const svtkStdString& password);

  svtkStdString GetQuery();
  void SetQuery(const svtkStdString& query);

  //@{
  /**
   * The name of the array for generating or assigning pedigree ids
   * (default "id").
   */
  svtkSetStringMacro(PedigreeIdArrayName);
  svtkGetStringMacro(PedigreeIdArrayName);
  //@}

  //@{
  /**
   * If on (default), generates pedigree ids automatically.
   * If off, assign one of the arrays to be the pedigree id.
   */
  svtkSetMacro(GeneratePedigreeIds, bool);
  svtkGetMacro(GeneratePedigreeIds, bool);
  svtkBooleanMacro(GeneratePedigreeIds, bool);
  //@}

protected:
  svtkSQLDatabaseTableSource();
  ~svtkSQLDatabaseTableSource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkSQLDatabaseTableSource(const svtkSQLDatabaseTableSource&) = delete;
  void operator=(const svtkSQLDatabaseTableSource&) = delete;

  char* PedigreeIdArrayName;
  bool GeneratePedigreeIds;

  /**
   * This intercepts events from the graph layout class
   * and re-emits them as if they came from this class.
   */
  svtkEventForwarderCommand* EventForwarder;

  class implementation;
  implementation* const Implementation;
};

#endif

// SVTK-HeaderTest-Exclude: svtkSQLDatabaseTableSource.h
