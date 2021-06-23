/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFixedWidthTextReader.cxx

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

#include <svtkFixedWidthTextReader.h>
#include <svtkIOStream.h>
#include <svtkNew.h>
#include <svtkStringArray.h>
#include <svtkTable.h>
#include <svtkTestErrorObserver.h>
#include <svtkTestUtilities.h>
#include <svtkVariant.h>
#include <svtkVariantArray.h>

int TestFixedWidthTextReader(int argc, char* argv[])
{
  std::cout << "### Pass 1: No headers, field width 10, do not strip whitespace" << std::endl;

  svtkIdType i, j;
  char* filename = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/fixedwidth.txt");

  std::cout << "Filename: " << filename << std::endl;

  svtkNew<svtkTest::ErrorObserver> errorObserver1;

  svtkFixedWidthTextReader* reader = svtkFixedWidthTextReader::New();
  reader->SetHaveHeaders(false);
  reader->SetFieldWidth(10);
  reader->StripWhiteSpaceOff();
  reader->SetFileName(filename);
  reader->SetTableErrorObserver(errorObserver1);
  reader->Update();
  int status = errorObserver1->CheckErrorMessage(
    "Incorrect number of tuples in SetRow. Expected 4, but got 6");
  std::cout << "Printing reader info..." << std::endl;
  reader->Print(std::cout);

  svtkTable* table = reader->GetOutput();

  std::cout << "FixedWidth text file has " << table->GetNumberOfRows() << " rows" << std::endl;
  std::cout << "FixedWidth text file has " << table->GetNumberOfColumns() << " columns"
            << std::endl;
  std::cout << "Column names: " << std::endl;

  for (i = 0; i < table->GetNumberOfColumns(); ++i)
  {
    std::cout << "\tColumn " << i << ": " << table->GetColumn(i)->GetName() << std::endl;
  }

  std::cout << "Table contents:" << std::endl;

  for (i = 0; i < table->GetNumberOfRows(); ++i)
  {
    svtkVariantArray* row = table->GetRow(i);

    for (j = 0; j < row->GetNumberOfTuples(); ++j)
    {
      std::cout << "Row " << i << " column " << j << ": ";

      svtkVariant value = row->GetValue(j);
      if (!value.IsValid())
      {
        std::cout << "invalid value" << std::endl;
      }
      else
      {
        std::cout << "type " << value.GetTypeAsString() << " value " << value.ToString()
                  << std::endl;
      }
    }
  }

  reader->Delete();
  delete[] filename;

  reader = svtkFixedWidthTextReader::New();
  filename = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/fixedwidth.txt");

  reader->HaveHeadersOn();
  reader->SetFieldWidth(10);
  reader->StripWhiteSpaceOn();
  reader->SetFileName(filename);
  reader->SetTableErrorObserver(errorObserver1);
  reader->Update();
  status += errorObserver1->CheckErrorMessage(
    "Incorrect number of tuples in SetRow. Expected 4, but got 6");
  table = reader->GetOutput();

  std::cout << std::endl << "### Test 2: headers, field width 10, strip whitespace" << std::endl;

  std::cout << "Printing reader info..." << std::endl;
  reader->Print(std::cout);

  std::cout << "FixedWidth text file has " << table->GetNumberOfRows() << " rows" << std::endl;
  std::cout << "FixedWidth text file has " << table->GetNumberOfColumns() << " columns"
            << std::endl;
  std::cout << "Column names: " << std::endl;
  for (i = 0; i < table->GetNumberOfColumns(); ++i)
  {
    std::cout << "\tColumn " << i << ": " << table->GetColumn(i)->GetName() << std::endl;
  }

  std::cout << "Table contents:" << std::endl;

  for (i = 0; i < table->GetNumberOfRows(); ++i)
  {
    svtkVariantArray* row = table->GetRow(i);

    for (j = 0; j < row->GetNumberOfTuples(); ++j)
    {
      std::cout << "Row " << i << " column " << j << ": ";

      svtkVariant value = row->GetValue(j);
      if (!value.IsValid())
      {
        std::cout << "invalid value" << std::endl;
      }
      else
      {
        std::cout << "type " << value.GetTypeAsString() << " value " << value.ToString()
                  << std::endl;
      }
    }
  }

  reader->Delete();
  delete[] filename;

  return 0;
}
