/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMergeTables.cxx

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

#include <svtkDelimitedTextReader.h>
#include <svtkIOStream.h>
#include <svtkMergeTables.h>
#include <svtkSmartPointer.h>
#include <svtkStringArray.h>
#include <svtkTable.h>
#include <svtkTestUtilities.h>
#include <svtkVariant.h>
#include <svtkVariantArray.h>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

int TestMergeTables(int argc, char* argv[])
{
  char* filename1 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/merge1.csv");
  char* filename2 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/merge2.csv");

  SVTK_CREATE(svtkDelimitedTextReader, reader1);
  reader1->SetFieldDelimiterCharacters(",");
  reader1->SetFileName(filename1);
  reader1->SetHaveHeaders(true);
  reader1->Update();

  svtkTable* table1 = reader1->GetOutput();

  SVTK_CREATE(svtkDelimitedTextReader, reader2);
  reader2->SetFieldDelimiterCharacters(",");
  reader2->SetFileName(filename2);
  reader2->SetHaveHeaders(true);
  reader2->Update();

  svtkTable* table2 = reader2->GetOutput();

  cout << "Table 1:" << endl;
  table1->Dump(10);

  cout << "Table 2:" << endl;
  table2->Dump(10);

  SVTK_CREATE(svtkMergeTables, merge);
  merge->SetInputData(0, table1);
  merge->SetInputData(1, table2);
  merge->SetMergeColumnsByName(true);
  merge->Update();

  svtkTable* mergedTable = merge->GetOutput();

  cout << "Merged Table:" << endl;
  mergedTable->Dump(10);

  // Test # of columns
  // - There should be 3: Col1, Col2, Col3
  if (mergedTable->GetNumberOfColumns() != 3)
  {
    cout << "ERROR: Wrong number of columns!" << endl
         << "       Expected 3, got " << mergedTable->GetNumberOfColumns() << endl;
    return 1;
  }

  delete[] filename1;
  delete[] filename2;

  return 0;
}
