/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestISIReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkISIReader.h"
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

int TestISIReader(int argc, char* argv[])
{
  char* file = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/eg2.isi");

  cerr << "file: " << file << endl;

  svtkSmartPointer<svtkISIReader> reader = svtkSmartPointer<svtkISIReader>::New();
  reader->SetFileName(file);
  delete[] file;
  reader->Update();
  svtkTable* const table = reader->GetOutput();

  int error_count = 0;

  // Test the size of the output table ...
  TestValue(table->GetNumberOfColumns(), svtkIdType(37), "Column count", error_count);
  TestValue(table->GetNumberOfRows(), svtkIdType(501), "Row count", error_count);

  // Test a sampling of the table columns ...
  TestValue(svtkStdString(table->GetColumnName(0)), svtkStdString("PT"), "Column 0", error_count);
  TestValue(svtkStdString(table->GetColumnName(1)), svtkStdString("AU"), "Column 1", error_count);
  TestValue(svtkStdString(table->GetColumnName(2)), svtkStdString("TI"), "Column 2", error_count);
  TestValue(svtkStdString(table->GetColumnName(20)), svtkStdString("PD"), "Column 20", error_count);
  TestValue(svtkStdString(table->GetColumnName(21)), svtkStdString("PY"), "Column 21", error_count);
  TestValue(svtkStdString(table->GetColumnName(22)), svtkStdString("VL"), "Column 22", error_count);
  TestValue(svtkStdString(table->GetColumnName(34)), svtkStdString("DE"), "Column 34", error_count);
  TestValue(svtkStdString(table->GetColumnName(35)), svtkStdString("SI"), "Column 35", error_count);
  TestValue(svtkStdString(table->GetColumnName(36)), svtkStdString("PN"), "Column 36", error_count);

  // Test a sampling of the table values ...
  TestValue(table->GetValue(0, 0).ToString(), svtkStdString("J"), "Value 0, 0", error_count);
  TestValue(table->GetValue(0, 1).ToString(), svtkStdString("Arantes, GM;Chaimovich, H"),
    "Value 0, 1", error_count);
  TestValue(table->GetValue(0, 2).ToString(),
    svtkStdString("Thiolysis and alcoholysis of phosphate tri- and monoesters with alkyl;and aryl "
                 "leaving groups. An ab initio study in the gas phase"),
    "Value 0, 2", error_count);

  TestValue(
    table->GetValue(499, 20).ToString(), svtkStdString("JAN 30"), "value 499, 20", error_count);
  TestValue(
    table->GetValue(499, 21).ToString(), svtkStdString("1996"), "value 499, 21", error_count);
  TestValue(table->GetValue(499, 22).ToString(), svtkStdString("17"), "value 499, 22", error_count);

  return error_count;
}
