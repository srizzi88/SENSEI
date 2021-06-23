/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRISReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRISReader.h"
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

int TestRISReader(int argc, char* argv[])
{
  char* file = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/eg1.ris");

  cerr << "file: " << file << endl;

  svtkSmartPointer<svtkRISReader> reader = svtkSmartPointer<svtkRISReader>::New();
  reader->SetFileName(file);
  delete[] file;

  reader->Update();
  svtkTable* const table = reader->GetOutput();

  int error_count = 0;

  // Test the size of the output table ...
  TestValue(table->GetNumberOfColumns(), svtkIdType(16), "Column count", error_count);
  TestValue(table->GetNumberOfRows(), svtkIdType(14), "Row count", error_count);

  // Test a sampling of the table columns ...
  TestValue(svtkStdString(table->GetColumnName(0)), svtkStdString("TY"), "Column 0", error_count);
  TestValue(svtkStdString(table->GetColumnName(1)), svtkStdString("T1"), "Column 1", error_count);
  TestValue(svtkStdString(table->GetColumnName(2)), svtkStdString("JF"), "Column 2", error_count);
  TestValue(svtkStdString(table->GetColumnName(13)), svtkStdString("KW"), "Column 13", error_count);
  TestValue(svtkStdString(table->GetColumnName(14)), svtkStdString("N1"), "Column 14", error_count);
  TestValue(svtkStdString(table->GetColumnName(15)), svtkStdString("UR"), "Column 15", error_count);

  // Test a sampling of the table values ...
  TestValue(table->GetValue(0, 0).ToString(), svtkStdString("JOUR"), "Value 0, 0", error_count);
  TestValue(table->GetValue(0, 1).ToString(),
    svtkStdString("Laser frequency stabilization at 1.5 microns using ultranarrow inhomogeneous "
                 "absorption profiles in Er3+:LiYF4"),
    "Value 0, 1", error_count);
  TestValue(table->GetValue(0, 2).ToString(), svtkStdString("Journal of Luminescence"), "Value 0, 2",
    error_count);

  TestValue(table->GetValue(13, 10).ToString(),
    svtkStdString("Zhou, P.;Li, X.-H.;Liang, Y.-J.;Deng, F.-G.;Zhou, H.-Y."), "value 13, 10",
    error_count);
  TestValue(table->GetValue(13, 11).ToString(),
    svtkStdString(
      "Key Laboratory of Beam Technology and Material Modification, Ministry of Education, Beijing "
      "Normal University, Beijing, 100875, China;Institute of Low Energy Nuclear Physics, "
      "Department of Material Science and Engineering, Beijing Normal University, Beijing, 100875, "
      "China;Beijing Radiation Center, Beijing, 100875, China"),
    "value 13, 11", error_count);
  TestValue(table->GetValue(13, 13).ToString(),
    svtkStdString(
      "Decoy photons;Pure entangled states;Quantum communication;Quantum secret sharing"),
    "value 13, 13", error_count);

  return error_count;
}
