/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHighestDensityRegionsStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDoubleArray.h"
#include "svtkHighestDensityRegionsStatistics.h"
#include "svtkIntArray.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"

#include <sstream>

//----------------------------------------------------------------------------
int TestHighestDensityRegionsStatistics(int, char*[])
{
  svtkNew<svtkTable> table;

  svtkNew<svtkDoubleArray> arrFirstVariable;
  const char* namev1 = "Math";
  arrFirstVariable->SetName(namev1);
  table->AddColumn(arrFirstVariable);

  svtkNew<svtkDoubleArray> arrSecondVariable;
  const char* namev2 = "French";
  arrSecondVariable->SetName(namev2);
  table->AddColumn(arrSecondVariable);

  svtkNew<svtkDoubleArray> arrThirdVariable;
  const char* namev3 = "MG";
  arrThirdVariable->SetName(namev3);
  table->AddColumn(arrThirdVariable);

  int numPoints = 20;
  table->SetNumberOfRows(numPoints);

  double MathValue[] = {
    18, 20, 20, 16, //
    12, 14, 16, 14, //
    14, 13, 16, 18, //
    6, 10, 16, 14,  //
    4, 16, 16, 14   //
  };

  double FrenchValue[] = {
    14, 12, 14, 16, //
    12, 14, 16, 4,  //
    4, 10, 6, 20,   //
    14, 16, 14, 14, //
    12, 2, 14, 8    //
  };

  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i); // Known Test Values
    table->SetValue(i, 1, MathValue[i]);
    table->SetValue(i, 2, FrenchValue[i]);
    table->SetValue(i, 3, (MathValue[i] + FrenchValue[i]) / 2.0);
    table->SetValue(i, 4, MathValue[i] - FrenchValue[i]);
  }

  // Run HDR
  // Set HDR statistics algorithm and its input data port
  svtkNew<svtkHighestDensityRegionsStatistics> hdrs;

  // First verify that absence of input does not cause trouble
  cout << "## Verifying that absence of input does not cause trouble... ";
  hdrs->Update();
  cout << "done.\n";

  hdrs->SetInputData(svtkStatisticsAlgorithm::INPUT_DATA, table);
  // Select Column Pairs of Interest ( Learn Mode )
  // 1: a valid pair
  hdrs->AddColumnPair(namev1, namev2);
  // 2: another valid pair
  hdrs->AddColumnPair(namev2, namev3);
  // 3: an invalid pair
  hdrs->AddColumnPair(namev2, "M3");

  hdrs->SetLearnOption(true);
  hdrs->SetDeriveOption(true);
  hdrs->SetAssessOption(false);
  hdrs->SetTestOption(false);
  hdrs->Update();

  cout << "\n## Result:\n";
  svtkMultiBlockDataSet* outputMetaDS = svtkMultiBlockDataSet::SafeDownCast(
    hdrs->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));

  svtkTable* outputMetaLearn = svtkTable::SafeDownCast(outputMetaDS->GetBlock(0));
  outputMetaLearn->Dump();

  std::stringstream ss;
  ss << "HDR (" << namev1 << "," << namev2 << ")";
  svtkDoubleArray* HDRArray =
    svtkArrayDownCast<svtkDoubleArray>(outputMetaLearn->GetColumnByName(ss.str().c_str()));
  if (!HDRArray)
  {
    cout << "Fail! The HDR column is missing from the result table!" << endl;
    return EXIT_FAILURE;
  }
  cout << "## Done." << endl;

  return EXIT_SUCCESS;
}
