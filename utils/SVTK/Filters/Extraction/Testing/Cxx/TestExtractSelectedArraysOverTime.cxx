/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelectedArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractSelectedArraysOverTime.h"

#include "svtkExodusIIReader.h"
#include "svtkExtractSelection.h"
#include "svtkExtractTimeSteps.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkSelectionNode.h"
#include "svtkSelectionSource.h"
#include "svtkTable.h"
#include "svtkTestUtilities.h"

#define expect(x, msg)                                                                             \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << __LINE__ << ": " msg << endl;                                                          \
    return false;                                                                                  \
  }

namespace
{
bool Validate0(svtkMultiBlockDataSet* mb, int num_timesteps)
{
  expect(mb != nullptr, "expecting a svtkMultiBlockDataSet.");
  expect(mb->GetNumberOfBlocks() == 1, "expecting 1 blocks, got " << mb->GetNumberOfBlocks());

  svtkTable* b0 = svtkTable::SafeDownCast(mb->GetBlock(0));
  expect(b0 != nullptr, "expecting a svtkTable for block 0");
  expect(b0->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows());
  expect(b0->GetColumnByName("avg(EQPS)") != nullptr, "missing 'avg(EQPS)'.");
  expect(b0->GetColumnByName("max(EQPS)") != nullptr, "missing 'max(EQPS)'.");
  expect(b0->GetColumnByName("min(EQPS)") != nullptr, "missing 'min(EQPS)'.");
  expect(b0->GetColumnByName("min(EQPS)") != nullptr, "missing 'min(EQPS)'.");
  expect(b0->GetColumnByName("q1(EQPS)") != nullptr, "missing 'q1(EQPS)'.");
  expect(b0->GetColumnByName("q3(EQPS)") != nullptr, "missing 'q3(EQPS)'.");
  expect(b0->GetColumnByName("N") != nullptr, "missing 'N'.");
  return true;
}

bool Validate1(svtkMultiBlockDataSet* mb, int num_timesteps)
{
  expect(mb != nullptr, "expecting a svtkMultiBlockDataSet.");
  expect(mb->GetNumberOfBlocks() == 3, "expecting 3 block, got " << mb->GetNumberOfBlocks());

  svtkTable* b0 = svtkTable::SafeDownCast(mb->GetBlock(0));
  expect(b0 != nullptr, "expecting a svtkTable for block 0");
  expect(b0->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows());
  expect(b0->GetNumberOfColumns() >= 5, "mismatched columns");
  expect(b0->GetColumnByName("EQPS") != nullptr, "missing 'EQPS'");
  expect(b0->GetColumnByName("Time") != nullptr, "missing 'Time'");

  const char* name = mb->GetMetaData(0u)->Get(svtkCompositeDataSet::NAME());
  expect(name != nullptr, "expecting non-null name.");
  expect(strcmp(name, "gid=786") == 0,
    "block name not matching, expected 'gid=786', got '" << name << "'");

  svtkTable* b1 = svtkTable::SafeDownCast(mb->GetBlock(1));
  expect(b1 != nullptr, "expecting a svtkTable for block 0");
  expect(b1->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b1->GetNumberOfRows());
  expect(b1->GetNumberOfColumns() >= 5, "mismatched columns");
  expect(b1->GetColumnByName("EQPS") != nullptr, "missing 'EQPS'");
  expect(b1->GetColumnByName("Time") != nullptr, "missing 'Time'");

  name = mb->GetMetaData(1u)->Get(svtkCompositeDataSet::NAME());
  expect(name != nullptr, "expecting non-null name.");
  expect(strcmp(name, "gid=787") == 0,
    "block name not matching, expected 'gid=787', got '" << name << "'");

  svtkTable* b2 = svtkTable::SafeDownCast(mb->GetBlock(2));
  expect(b2 != nullptr, "expecting a svtkTable for block 0");
  expect(b2->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b2->GetNumberOfRows());
  expect(b2->GetNumberOfColumns() >= 5, "mismatched columns");
  expect(b2->GetColumnByName("EQPS") != nullptr, "missing 'EQPS'");
  expect(b2->GetColumnByName("Time") != nullptr, "missing 'Time'");

  name = mb->GetMetaData(2u)->Get(svtkCompositeDataSet::NAME());
  expect(name != nullptr, "expecting non-null name.");
  expect(strcmp(name, "gid=788") == 0,
    "block name not matching, expected 'gid=788', got '" << name << "'");
  return true;
}
}

int TestExtractSelectedArraysOverTime(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");

  svtkNew<svtkExodusIIReader> reader;
  reader->SetFileName(fname);
  reader->UpdateInformation();
  reader->SetAllArrayStatus(svtkExodusIIReader::NODAL, 1);
  reader->SetAllArrayStatus(svtkExodusIIReader::ELEM_BLOCK, 1);
  reader->SetGenerateGlobalElementIdArray(true);
  reader->SetGenerateGlobalNodeIdArray(true);
  delete[] fname;

  // lets limit to 10 timesteps to reduce test time.
  svtkNew<svtkExtractTimeSteps> textracter;
  textracter->SetInputConnection(reader->GetOutputPort());
  textracter->UpdateInformation();
  textracter->GenerateTimeStepIndices(0, 3, 1);
  const int num_timesteps = 3;

  svtkNew<svtkSelectionSource> selSource;
  selSource->SetContentType(svtkSelectionNode::GLOBALIDS);
  selSource->SetFieldType(svtkSelectionNode::CELL);
  selSource->AddID(0, 786);
  selSource->AddID(0, 787);
  selSource->AddID(0, 788);

  svtkNew<svtkExtractSelectedArraysOverTime> extractor;
  extractor->SetInputConnection(0, textracter->GetOutputPort());
  extractor->SetInputConnection(1, selSource->GetOutputPort());
  extractor->SetReportStatisticsOnly(true);
  extractor->Update();

  if (!Validate0(
        svtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)), num_timesteps))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  extractor->SetReportStatisticsOnly(false);
  extractor->Update();

  if (!Validate1(
        svtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)), num_timesteps))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
