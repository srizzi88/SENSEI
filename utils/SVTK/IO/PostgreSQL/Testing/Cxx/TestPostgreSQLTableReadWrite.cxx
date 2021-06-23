/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPostgreSQLTableReadWrite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkTableToPostgreSQLWriter and svtkPostgreSQLToTableReader
// .SECTION Description
//

#include "svtkPostgreSQLDatabase.h"
#include "svtkSQLQuery.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkTableReader.h"
#include "svtkTableWriter.h"
#include "svtkToolkits.h"
#include "svtksys/SystemTools.hxx"

#include "svtkIOPostgresSQLTestingCxxConfigure.h"
#include "svtkPostgreSQLToTableReader.h"
#include "svtkTableToPostgreSQLWriter.h"

int TestPostgreSQLTableReadWrite(int argc, char* argv[])
{
  if (argc <= 1)
  {
    cerr << "Usage: " << argv[0] << " <.svtk table file>" << endl;
    return 1;
  }
  cerr << "reading a svtkTable from file" << endl;
  svtkSmartPointer<svtkTableReader> tableFileReader = svtkSmartPointer<svtkTableReader>::New();
  tableFileReader->SetFileName(argv[1]);
  svtkTable* table = tableFileReader->GetOutput();
  tableFileReader->Update();

  cerr << "opening a PostgreSQL database connection" << endl;

  svtkPostgreSQLDatabase* db =
    svtkPostgreSQLDatabase::SafeDownCast(svtkSQLDatabase::CreateFromURL(SVTK_PSQL_TEST_URL));
  svtkStdString realDatabase = db->GetDatabaseName();
  db->SetDatabaseName("template1"); // This is guaranteed to exist
  bool status = db->Open();
  if (!status)
  {
    cerr << "Couldn't open database.\n";
    return 1;
  }

  if (!db->CreateDatabase(realDatabase.c_str(), true))
  {
    cerr << "Error: " << db->GetLastErrorText() << endl;
  }
  db->SetDatabaseName(realDatabase.c_str());
  if (!db->Open())
  {
    cerr << "Error: " << db->GetLastErrorText() << endl;
    return 1;
  }

  cerr << "creating a PostgreSQL table from a svtkTable" << endl;
  svtkSmartPointer<svtkTableToPostgreSQLWriter> writerToTest =
    svtkSmartPointer<svtkTableToPostgreSQLWriter>::New();

  writerToTest->SetInputData(table);
  writerToTest->SetDatabase(db);
  writerToTest->SetTableName("tabletest");
  writerToTest->Update();

  cerr << "converting it back to a svtkTable" << endl;
  svtkSmartPointer<svtkPostgreSQLToTableReader> readerToTest =
    svtkSmartPointer<svtkPostgreSQLToTableReader>::New();

  readerToTest->SetDatabase(db);
  readerToTest->SetTableName("tabletest");
  readerToTest->Update();

  cerr << "writing the table out to disk" << endl;
  svtkSmartPointer<svtkTableWriter> tableFileWriter = svtkSmartPointer<svtkTableWriter>::New();
  tableFileWriter->SetFileName("TestPostgreSQLTableReadWrite.svtk");
  tableFileWriter->SetInputConnection(readerToTest->GetOutputPort());
  tableFileWriter->Update();

  cerr << "verifying that it's the same as what we started with...";
  int result = 0;
  if (svtksys::SystemTools::FilesDiffer(argv[1], "TestPostgreSQLTableReadWrite.svtk"))
  {
    cerr << "it's not." << endl;
    result = 1;
  }
  else
  {
    cerr << "it is!" << endl;
  }

  // drop the table we created
  svtkSQLQuery* query = db->GetQueryInstance();
  query->SetQuery("DROP TABLE tabletest");
  query->Execute();

  cerr << "dropping the database...";

  if (!db->DropDatabase(realDatabase.c_str()))
  {
    cout << "Drop of \"" << realDatabase.c_str() << "\" failed.\n";
    cerr << "\"" << db->GetLastErrorText() << "\"" << endl;
  }

  // clean up memory
  db->Delete();
  query->Delete();

  return result;
}
