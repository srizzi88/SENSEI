/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractDataArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractDataArraysOverTime.h"

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
  expect(mb->GetNumberOfBlocks() == 2, "expecting 2 blocks, got " << mb->GetNumberOfBlocks());

  svtkTable* b0 = svtkTable::SafeDownCast(mb->GetBlock(0));
  expect(b0 != nullptr, "expecting a svtkTable for block 0");
  expect(b0->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows());
  expect(b0->GetNumberOfColumns() > 100, "mismatched columns");

  svtkTable* b1 = svtkTable::SafeDownCast(mb->GetBlock(1));
  expect(b1 != nullptr, "expecting a svtkTable for block 1");
  expect(b1->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b1->GetNumberOfRows());
  expect(b1->GetNumberOfColumns() > 100, "mismatched columns");
  return true;
}

bool Validate1(svtkMultiBlockDataSet* mb, int num_timesteps, const char* bname)
{
  expect(mb != nullptr, "expecting a svtkMultiBlockDataSet.");
  expect(mb->GetNumberOfBlocks() == 1, "expecting 1 block, got " << mb->GetNumberOfBlocks());

  svtkTable* b0 = svtkTable::SafeDownCast(mb->GetBlock(0));
  expect(b0 != nullptr, "expecting a svtkTable for block 0");
  expect(b0->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows());
  expect(b0->GetNumberOfColumns() >= 5, "mismatched columns");

  const char* name = mb->GetMetaData(0u)->Get(svtkCompositeDataSet::NAME());
  expect(name != nullptr, "expecting non-null name.");
  expect(strcmp(name, bname) == 0,
    "block name not matching,"
    " expected '"
      << bname << "', got '" << name << "'");
  return true;
}
}

int TestExtractDataArraysOverTime(int argc, char* argv[])
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
  textracter->GenerateTimeStepIndices(1, 11, 1);
  const int num_timesteps = 10;

  svtkNew<svtkExtractDataArraysOverTime> extractor;
  extractor->SetReportStatisticsOnly(true);
  extractor->SetInputConnection(textracter->GetOutputPort());
  extractor->Update();

  if (!Validate0(
        svtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)), num_timesteps))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  // let's try non-summary extraction.

  svtkNew<svtkSelectionSource> selSource;
  selSource->SetContentType(svtkSelectionNode::GLOBALIDS);
  selSource->SetFieldType(svtkSelectionNode::CELL);
  selSource->AddID(0, 100);

  svtkNew<svtkExtractSelection> iextractor;
  iextractor->SetInputConnection(0, textracter->GetOutputPort());
  iextractor->SetInputConnection(1, selSource->GetOutputPort());

  extractor->SetReportStatisticsOnly(false);
  extractor->SetInputConnection(iextractor->GetOutputPort());
  extractor->SetFieldAssociation(svtkDataObject::CELL);
  extractor->Update();
  if (!Validate1(svtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
        num_timesteps, "gid=100"))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  // this time, simply use element id.
  extractor->SetUseGlobalIDs(false);
  extractor->Update();
  if (!Validate1(svtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
        num_timesteps, "originalId=99 block=2"))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  // this time, request using svtkOriginalCellIds to id the elements.
  extractor->SetUseGlobalIDs(false);
  extractor->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, "svtkOriginalCellIds");
  extractor->Update();
  if (!Validate1(svtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
        num_timesteps, "originalId=99 block=2"))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
