/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBoostSplitTableField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBoostSplitTableField.h"
#include "svtkDelimitedTextReader.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkTable.h"
#include "svtkTestUtilities.h"
#include "svtkVariant.h"

template <typename value_t>
void TestValue(const value_t& Value, const value_t& ExpectedValue,
  const svtkStdString& ValueDescription, int& ErrorCount)
{
  if (Value == ExpectedValue)
    return;

  cerr << ValueDescription << " is [" << Value << "] - expected [" << ExpectedValue << "]" << endl;

  ++ErrorCount;
}

int TestBoostSplitTableField(int argc, char* argv[])
{
  char* file = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/publications.csv");

  cerr << "file: " << file << endl;

  svtkSmartPointer<svtkDelimitedTextReader> reader = svtkSmartPointer<svtkDelimitedTextReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->SetHaveHeaders(true);

  svtkSmartPointer<svtkBoostSplitTableField> split = svtkSmartPointer<svtkBoostSplitTableField>::New();
  split->AddInputConnection(reader->GetOutputPort());
  split->AddField("Author", ";");

  split->Update();
  svtkTable* const table = split->GetOutput();

  int error_count = 0;

  // Test the size of the output table ...
  TestValue(table->GetNumberOfColumns(), svtkIdType(5), "Column count", error_count);
  TestValue(table->GetNumberOfRows(), svtkIdType(9), "Row count", error_count);

  // Test a sampling of the table columns ...
  TestValue(svtkStdString(table->GetColumnName(0)), svtkStdString("PubID"), "Column 0", error_count);
  TestValue(svtkStdString(table->GetColumnName(1)), svtkStdString("Author"), "Column 1", error_count);
  TestValue(
    svtkStdString(table->GetColumnName(2)), svtkStdString("Journal"), "Column 2", error_count);
  TestValue(
    svtkStdString(table->GetColumnName(3)), svtkStdString("Categories"), "Column 3", error_count);
  TestValue(
    svtkStdString(table->GetColumnName(4)), svtkStdString("Accuracy"), "Column 4", error_count);

  // Test a sampling of the table values ...
  TestValue(table->GetValue(0, 0).ToString(), svtkStdString("P001"), "Value 0, 0", error_count);
  TestValue(table->GetValue(0, 1).ToString(), svtkStdString("Biff"), "Value 0, 1", error_count);
  TestValue(table->GetValue(0, 2).ToString(), svtkStdString("American Journal of Spacecraft Music"),
    "Value 0, 2", error_count);

  TestValue(table->GetValue(7, 0).ToString(), svtkStdString("P008"), "value 7, 0", error_count);
  TestValue(table->GetValue(7, 1).ToString(), svtkStdString("Biff"), "value 7, 1", error_count);
  TestValue(table->GetValue(7, 2).ToString(),
    svtkStdString("American Crafts and Holistic Medicine Quarterly"), "value 7, 2", error_count);

  TestValue(table->GetValue(8, 0).ToString(), svtkStdString("P008"), "value 8, 0", error_count);
  TestValue(table->GetValue(8, 1).ToString(), svtkStdString("Bob"), "value 8, 1", error_count);
  TestValue(table->GetValue(8, 2).ToString(),
    svtkStdString("American Crafts and Holistic Medicine Quarterly"), "value 8, 2", error_count);

  return error_count;
}
