
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestODBCDatabase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
  Copyright (c) Sandia Corporation
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
  ----------------------------------------------------------------------------*/
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for implementing
// this test.

#include "svtkIOODBCTestingCxxConfigure.h"
#include "svtkODBCDatabase.h"
#include "svtkRowQueryToTable.h"
#include "svtkSQLQuery.h"
#include "svtkStdString.h"
#include "svtkTable.h"
#include "svtkToolkits.h"
#include "svtkVariant.h"
#include "svtkVariantArray.h"

#include <sstream>

#define LONGSTRING "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"

int TestODBCDatabase(int, char** const)
{
  svtkODBCDatabase* db = svtkODBCDatabase::New();
  db->SetDataSourceName(SVTK_ODBC_TEST_DSN);
  bool status = db->Open(nullptr);

  if (!status)
  {
    cerr << "Couldn't open database.  Error message: " << db->GetLastErrorText() << "\n";
    return 1;
  }

  svtkSQLQuery* query = db->GetQueryInstance();

  svtkStdString createQuery("CREATE TABLE people (name VARCHAR(1024), age INTEGER, weight FLOAT)");
  cout << createQuery << endl;
  query->SetQuery(createQuery.c_str());
  if (!query->Execute())
  {
    cerr << "Create query failed.  Error message: " << query->GetLastErrorText() << endl;
    return 1;
  }

  int i;
  for (i = 0; i < 20; ++i)
  {
    std::ostringstream queryBuf;

    queryBuf << "INSERT INTO people VALUES('John Doe " << i << "', " << i << ", " << 10 * i + 0.5
             << ")";
    cout << queryBuf.str() << endl;
    query->SetQuery(queryBuf.str().c_str());
    if (!query->Execute())
    {
      cerr << "Insert query " << i << " failed.  Error message: " << query->GetLastErrorText()
           << endl;
      return 1;
    }
  }

  const char* placeholders = "INSERT INTO people (name, age, weight) VALUES (?, ?, ?)";
  query->SetQuery(placeholders);
  for (i = 21; i < 40; i++)
  {
    char name[20];
    snprintf(name, sizeof(name), "John Doe %d", i);
    bool bind1 = query->BindParameter(0, name);
    bool bind2 = query->BindParameter(1, i);
    bool bind3 = query->BindParameter(2, 10.1 * i);
    if (!(bind1 && bind2 && bind3))
    {
      cerr << "Parameter binding failed on query " << i << ": " << bind1 << " " << bind2 << " "
           << bind3 << endl;
      return 1;
    }
    cout << query->GetQuery() << endl;
    if (!query->Execute())
    {
      cerr << "Insert query " << i << " failed" << endl;
      return 1;
    }
  }

  const char* queryText = "SELECT name, age, weight FROM people WHERE age <= 30";
  query->SetQuery(queryText);
  cerr << endl << "Running query: " << query->GetQuery() << endl;

  cerr << endl << "Using svtkSQLQuery directly to execute query:" << endl;
  if (!query->Execute())
  {
    cerr << "Query failed with error message " << query->GetLastErrorText() << endl;
    return 1;
  }

  cerr << "Fields returned by query: ";
  for (int col = 0; col < query->GetNumberOfFields(); ++col)
  {
    if (col > 0)
    {
      cerr << ", ";
    }
    cerr << query->GetFieldName(col);
  }
  cerr << endl;
  int thisRow = 0;
  while (query->NextRow())
  {
    cerr << "Row " << thisRow << ": ";
    ++thisRow;
    for (int field = 0; field < query->GetNumberOfFields(); ++field)
    {
      if (field > 0)
      {
        cerr << ", ";
      }
      cerr << query->DataValue(field).ToString().c_str();
    }
    cerr << endl;
  }

  cerr << endl << "Using svtkSQLQuery to execute query and retrieve by row:" << endl;
  if (!query->Execute())
  {
    cerr << "Query failed with error message " << query->GetLastErrorText() << endl;
    return 1;
  }
  for (int col = 0; col < query->GetNumberOfFields(); ++col)
  {
    if (col > 0)
    {
      cerr << ", ";
    }
    cerr << query->GetFieldName(col);
  }
  cerr << endl;
  svtkVariantArray* va = svtkVariantArray::New();
  while (query->NextRow(va))
  {
    for (int field = 0; field < va->GetNumberOfValues(); ++field)
    {
      if (field > 0)
      {
        cerr << ", ";
      }
      cerr << va->GetValue(field).ToString().c_str();
    }
    cerr << endl;
  }
  va->Delete();

  cerr << endl << "Using svtkRowQueryToTable to execute query:" << endl;
  svtkRowQueryToTable* reader = svtkRowQueryToTable::New();
  reader->SetQuery(query);
  reader->Update();
  svtkTable* table = reader->GetOutput();
  for (svtkIdType col = 0; col < table->GetNumberOfColumns(); ++col)
  {
    table->GetColumn(col)->Print(cerr);
  }
  cerr << endl;

#if defined(PRINT_TABLE_CONTENTS)
  for (svtkIdType row = 0; row < table->GetNumberOfRows(); ++row)
  {
    for (svtkIdType col = 0; col < table->GetNumberOfColumns(); ++col)
    {
      svtkVariant v = table->GetValue(row, col);
      cerr << "row " << row << ", col " << col << " - " << v.ToString() << " ( "
           << svtkImageScalarTypeNameMacro(v.GetType()) << " )" << endl;
    }
  }
#endif

  query->SetQuery("DROP TABLE people");
  query->Execute();

  reader->Delete();
  query->Delete();
  db->Delete();

  return 0;
}
