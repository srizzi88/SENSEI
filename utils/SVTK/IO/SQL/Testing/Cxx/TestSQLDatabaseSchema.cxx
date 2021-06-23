/*=========================================================================

Program:   Visualization Toolkit
Module:    TestSQLDatabaseSchema.cxx

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
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
// for implementing this test.

#include "DatabaseSchemaWith2Tables.h"
#include "svtkSQLDatabaseSchema.h"
#include "svtkStdString.h"

#include <set>

int TestSQLDatabaseSchema(int /*argc*/, char* /*argv*/[])
{
  bool status = true;

  // 1. Create the schema
  DatabaseSchemaWith2Tables schema;

  // 2. Check the schema

  // Define the correct (reference) columns and types
  std::set<svtkStdString> preNames;
  preNames.insert(svtkStdString("dropplpgsql"));
  preNames.insert(svtkStdString("loadplpgsql"));
  preNames.insert(svtkStdString("createsomefunction"));
  std::multiset<svtkStdString> preBackends;
  preBackends.insert(svtkStdString(SVTK_SQL_POSTGRESQL));
  preBackends.insert(svtkStdString(SVTK_SQL_POSTGRESQL));
  preBackends.insert(svtkStdString(SVTK_SQL_POSTGRESQL));

  // Loop over all preambles
  int numPre = schema->GetNumberOfPreambles();
  if (numPre != 3)
  {
    cerr << "Read " << numPre << " != 3 preamble in test schema.\n";
    status = false;
  }

  for (int preHandle = 0; preHandle < numPre; ++preHandle)
  {
    svtkStdString preName = schema->GetPreambleNameFromHandle(preHandle);
    cerr << "Preamble name: " << preName << "\n";

    std::set<svtkStdString>::iterator sit = preNames.find(preName);
    if (sit != preNames.end())
    {
      preNames.erase(sit);
    }
    else
    {
      cerr << "Could not retrieve preamble name " << preName << " from test schema.\n";
      status = false;
    }

    svtkStdString preBackend = schema->GetPreambleBackendFromHandle(preHandle);
    cerr << "Preamble backend: " << preBackend << "\n";

    std::multiset<svtkStdString>::iterator mit = preBackends.find(preBackend);
    if (mit != preBackends.end())
    {
      preBackends.erase(mit);
    }
    else
    {
      cerr << "Could not retrieve preamble backend " << preBackend << " from test schema.\n";
      status = false;
    }
  }

  // Define the correct (reference) columns and types
  std::set<svtkStdString> colNames;
  colNames.insert(svtkStdString("somenmbr"));
  colNames.insert(svtkStdString("somename"));
  colNames.insert(svtkStdString("tablekey"));
  std::set<int> colTypes;
  colTypes.insert(static_cast<int>(svtkSQLDatabaseSchema::BIGINT));
  colTypes.insert(static_cast<int>(svtkSQLDatabaseSchema::SERIAL));
  colTypes.insert(static_cast<int>(svtkSQLDatabaseSchema::VARCHAR));

  // Loop over all columns of the first table
  int tblHandle = 0;
  int numCol = schema->GetNumberOfColumnsInTable(tblHandle);
  if (numCol != 3)
  {
    cerr << "Read " << numCol << " != 3 columns in test schema.\n";
    status = false;
  }

  for (int colHandle = 0; colHandle < numCol; ++colHandle)
  {
    svtkStdString colName = schema->GetColumnNameFromHandle(tblHandle, colHandle);
    cerr << "Column name: " << colName << "\n";

    std::set<svtkStdString>::iterator sit = colNames.find(colName);
    if (sit != colNames.end())
    {
      colNames.erase(sit);
    }
    else
    {
      cerr << "Could not retrieve column name " << colName << " from test schema.\n";
      status = false;
    }

    int colType = schema->GetColumnTypeFromHandle(tblHandle, colHandle);
    cerr << "Column type: " << colType << "\n";

    std::set<int>::iterator iit = colTypes.find(colType);
    if (iit != colTypes.end())
    {
      colTypes.erase(iit);
    }
    else
    {
      cerr << "Could not retrieve column type " << colType << " from test schema.\n";
      status = false;
    }
  }

  // Define the correct (reference) indices and types
  std::set<svtkStdString> idxNames;
  idxNames.insert(svtkStdString("bigkey"));
  idxNames.insert(svtkStdString("reverselookup"));
  std::set<int> idxTypes;
  idxTypes.insert(static_cast<int>(svtkSQLDatabaseSchema::PRIMARY_KEY));
  idxTypes.insert(static_cast<int>(svtkSQLDatabaseSchema::UNIQUE));

  // Loop over all indices of the previously created table
  int numIdx = schema->GetNumberOfIndicesInTable(tblHandle);
  if (numIdx != 2)
  {
    cerr << "Read " << numIdx << " != 2 indices in test schema.\n";
    status = false;
  }

  for (int idxHandle = 0; idxHandle < numIdx; ++idxHandle)
  {
    svtkStdString idxName = schema->GetIndexNameFromHandle(tblHandle, idxHandle);
    cerr << "Index name: " << idxName << "\n";

    std::set<svtkStdString>::iterator sit = idxNames.find(idxName);
    if (sit != idxNames.end())
    {
      idxNames.erase(sit);
    }
    else
    {
      cerr << "Could not retrieve index name " << idxName << " from test schema.\n";
      status = false;
    }

    int idxType = schema->GetIndexTypeFromHandle(tblHandle, idxHandle);
    cerr << "Index type: " << idxType << "\n";

    std::set<int>::iterator iit = idxTypes.find(idxType);
    if (iit != idxTypes.end())
    {
      idxTypes.erase(iit);
    }
    else
    {
      cerr << "Could not retrieve index type " << idxType << " from test schema.\n";
      status = false;
    }
  }

  // Define the correct (reference) triggers and types
  std::multiset<svtkStdString> trgNames;
  trgNames.insert(svtkStdString("inserttrigger"));
  trgNames.insert(svtkStdString("inserttrigger"));
  trgNames.insert(svtkStdString("inserttrigger"));

  std::multiset<int> trgTypes;
  trgTypes.insert(static_cast<int>(svtkSQLDatabaseSchema::AFTER_INSERT));
  trgTypes.insert(static_cast<int>(svtkSQLDatabaseSchema::AFTER_INSERT));
  trgTypes.insert(static_cast<int>(svtkSQLDatabaseSchema::AFTER_INSERT));

  std::multiset<svtkStdString> trgActions;
  trgActions.insert(svtkStdString("DO NOTHING"));
  trgActions.insert(svtkStdString("FOR EACH ROW INSERT INTO btable SET somevalue = NEW.somenmbr"));
  trgActions.insert(svtkStdString("FOR EACH ROW EXECUTE PROCEDURE somefunction ()"));

  std::multiset<svtkStdString> trgBackends;
  trgBackends.insert(svtkStdString(SVTK_SQL_MYSQL));
  trgBackends.insert(svtkStdString(SVTK_SQL_SQLITE));
  trgBackends.insert(svtkStdString(SVTK_SQL_POSTGRESQL));

  // Loop over all triggers of the previously created table
  int numTrg = schema->GetNumberOfTriggersInTable(tblHandle);
  if (numTrg != 3)
  {
    cerr << "Read " << numTrg << " != 3 triggers in test schema.\n";
    status = false;
  }

  for (int trgHandle = 0; trgHandle < numTrg; ++trgHandle)
  {
    svtkStdString trgName = schema->GetTriggerNameFromHandle(tblHandle, trgHandle);
    cerr << "Trigger name: " << trgName << "\n";

    std::multiset<svtkStdString>::iterator sit = trgNames.find(trgName);
    if (sit != trgNames.end())
    {
      trgNames.erase(sit);
    }
    else
    {
      cerr << "Could not retrieve trigger name " << trgName << " from test schema.\n";
      status = false;
    }

    int trgType = schema->GetTriggerTypeFromHandle(tblHandle, trgHandle);
    cerr << "Trigger type: " << trgType << "\n";

    std::multiset<int>::iterator iit = trgTypes.find(trgType);
    if (iit != trgTypes.end())
    {
      trgTypes.erase(iit);
    }
    else
    {
      cerr << "Could not retrieve trigger type " << trgType << " from test schema.\n";
      status = false;
    }

    svtkStdString trgAction = schema->GetTriggerActionFromHandle(tblHandle, trgHandle);
    cerr << "Trigger action: " << trgAction << "\n";

    sit = trgActions.find(trgAction);
    if (sit != trgActions.end())
    {
      trgActions.erase(sit);
    }
    else
    {
      cerr << "Could not retrieve trigger action " << trgAction << " from test schema.\n";
      status = false;
    }

    svtkStdString trgBackend = schema->GetTriggerBackendFromHandle(tblHandle, trgHandle);
    cerr << "Trigger backend: " << trgBackend << "\n";

    sit = trgBackends.find(trgBackend);
    if (sit != trgBackends.end())
    {
      trgBackends.erase(sit);
    }
    else
    {
      cerr << "Could not retrieve trigger backend " << trgBackend << " from test schema.\n";
      status = false;
    }
  }

  return status ? 0 : 1;
}
