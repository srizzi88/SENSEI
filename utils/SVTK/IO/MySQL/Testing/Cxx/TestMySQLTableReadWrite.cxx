/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMySQLTableReadWrite.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkTableToMySQLWriter and svtkMySQLToTableReader
// .SECTION Description
//

#include "svtkMySQLDatabase.h"
#include "svtkSQLQuery.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkTableReader.h"
#include "svtkTableWriter.h"
#include "svtkToolkits.h"
#include "svtksys/SystemTools.hxx"

#include "svtkIOMySQLTestingCxxConfigure.h"
#include "svtkMySQLToTableReader.h"
#include "svtkTableToMySQLWriter.h"

int TestMySQLTableReadWrite(int argc, char* argv[])
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

  cerr << "opening a MySQL database connection" << endl;

  svtkMySQLDatabase* db =
    svtkMySQLDatabase::SafeDownCast(svtkSQLDatabase::CreateFromURL(SVTK_MYSQL_TEST_URL));
  bool status = db->Open();

  if (!status)
  {
    cerr << "Couldn't open database.\n";
    return 1;
  }

  cerr << "creating a MySQL table from a svtkTable" << endl;
  svtkSmartPointer<svtkTableToMySQLWriter> writerToTest =
    svtkSmartPointer<svtkTableToMySQLWriter>::New();

  writerToTest->SetInputData(table);
  writerToTest->SetDatabase(db);
  writerToTest->SetTableName("tableTest");
  writerToTest->Update();

  cerr << "converting it back to a svtkTable" << endl;
  svtkSmartPointer<svtkMySQLToTableReader> readerToTest =
    svtkSmartPointer<svtkMySQLToTableReader>::New();

  readerToTest->SetDatabase(db);
  readerToTest->SetTableName("tableTest");
  readerToTest->Update();

  cerr << "writing the table out to disk" << endl;
  svtkSmartPointer<svtkTableWriter> tableFileWriter = svtkSmartPointer<svtkTableWriter>::New();
  tableFileWriter->SetFileName("TestMySQLTableReadWrite.svtk");
  tableFileWriter->SetInputConnection(readerToTest->GetOutputPort());
  tableFileWriter->Update();

  cerr << "verifying that it's the same as what we started with...";
  int result = 0;
  if (svtksys::SystemTools::FilesDiffer(argv[1], "TestMySQLTableReadWrite.svtk"))
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
  query->SetQuery("DROP TABLE tableTest");
  query->Execute();

  // clean up memory
  db->Delete();
  query->Delete();

  return result;
}
