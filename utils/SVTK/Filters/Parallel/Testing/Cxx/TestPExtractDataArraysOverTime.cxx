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
#include "svtkMPI.h"
#include "svtkPExtractDataArraysOverTime.h"

#include "svtkExtractSelection.h"
#include "svtkExtractTimeSteps.h"
#include "svtkInformation.h"
#include "svtkMPIController.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPExodusIIReader.h"
#include "svtkSelectionNode.h"
#include "svtkSelectionSource.h"
#include "svtkTable.h"
#include "svtkTestUtilities.h"

#include <sstream>
#include <string>

#define expect(x, msg)                                                                             \
  if (!(x))                                                                                        \
  {                                                                                                \
    cerr << "rank=" << rank << ", line=" << __LINE__ << ": " msg << endl;                          \
    return false;                                                                                  \
  }

namespace
{
bool ValidateStats(svtkMultiBlockDataSet* mb, int num_timesteps, int rank)
{
  if (rank != 0)
  {
    // expecting MB with 2 empty blocks.
    expect(mb != nullptr, "expecting a svtkMultiBlockDataSet.");
    expect(mb->GetNumberOfBlocks() == 2, "expecting 2 blocks, got " << mb->GetNumberOfBlocks());
    for (int cc = 0; cc < 2; ++cc)
    {
      expect(mb->GetBlock(cc) == nullptr, "expecting null block at index : " << cc);
    }
    return true;
  }

  expect(mb != nullptr, "expecting a svtkMultiBlockDataSet.");
  expect(mb->GetNumberOfBlocks() == 2, "expecting 2 blocks, got " << mb->GetNumberOfBlocks());
  for (int cc = 0; cc < 2; ++cc)
  {
    svtkTable* b0 = svtkTable::SafeDownCast(mb->GetBlock(0));
    expect(b0 != nullptr, "expecting a svtkTable for block " << cc);
    expect(b0->GetNumberOfRows() == num_timesteps,
      "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows()
                                    << "for block " << cc);
    expect(b0->GetNumberOfColumns() > 100, "mismatched columns in block " << cc);
    expect(b0->GetColumnByName("max(DISPL (0))") != nullptr,
      "missing 'max(DISPL (0))' for block " << cc);
  }
  return true;
}

bool ValidateGID(svtkMultiBlockDataSet* mb, int num_timesteps, const char* bname, int rank)
{
  if (rank != 0)
  {
    // expecting MB with 1 empty blocks.
    expect(mb != nullptr, "expecting a svtkMultiBlockDataSet.");
    expect(mb->GetNumberOfBlocks() == 1, "expecting 1 blocks, got " << mb->GetNumberOfBlocks());
    expect(mb->GetBlock(0) == nullptr, "expecting null block at index 0.");
    return true;
  }

  expect(mb != nullptr, "expecting a svtkMultiBlockDataSet.");
  expect(mb->GetNumberOfBlocks() == 1, "expecting 1 block, got " << mb->GetNumberOfBlocks());

  svtkTable* b0 = svtkTable::SafeDownCast(mb->GetBlock(0));
  expect(b0 != nullptr, "expecting a svtkTable for block 0");
  expect(b0->GetNumberOfRows() == num_timesteps,
    "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows());
  expect(b0->GetNumberOfColumns() >= 5, "mismatched columns");
  expect(b0->GetColumnByName("EQPS") != nullptr, "missing EQPS.");

  const char* name = mb->GetMetaData(0u)->Get(svtkCompositeDataSet::NAME());
  expect(name != nullptr, "expecting non-null name.");
  expect(strcmp(name, bname) == 0,
    "block name not matching, expected '" << bname << "', got '" << name << "'");
  return true;
}

bool ValidateID(svtkMultiBlockDataSet* mb, int num_timesteps, const char* bname, int rank)
{
  if (rank != 0)
  {
    // expecting MB with 1 empty blocks.
    expect(mb != nullptr, "expecting a svtkMultiBlockDataSet.");
    expect(mb->GetNumberOfBlocks() == 1, "expecting 1 blocks, got " << mb->GetNumberOfBlocks());
    expect(mb->GetBlock(0) == nullptr, "expecting null block at index 0.");
    return true;
  }

  expect(mb != nullptr, "expecting a svtkMultiBlockDataSet.");
  expect(mb->GetNumberOfBlocks() == 1, "expecting 1 block, got " << mb->GetNumberOfBlocks());

  for (int cc = 0; cc < 1; ++cc)
  {
    svtkTable* b0 = svtkTable::SafeDownCast(mb->GetBlock(cc));
    expect(b0 != nullptr, "expecting a svtkTable for block " << cc);
    expect(b0->GetNumberOfRows() == num_timesteps,
      "mismatched rows, expecting " << num_timesteps << ", got " << b0->GetNumberOfRows());
    expect(b0->GetNumberOfColumns() >= 5, "mismatched columns");
    expect(b0->GetColumnByName("EQPS") != nullptr, "missing EQPS.");

    const char* name = mb->GetMetaData(cc)->Get(svtkCompositeDataSet::NAME());
    expect(name != nullptr, "expecting non-null name.");

    std::ostringstream stream;
    stream << bname << " rank=" << cc;
    expect(stream.str() == name,
      "block name not matching, expected '" << stream.str() << "', got '" << name << "'");
  }
  return true;
}

class Initializer
{
public:
  Initializer(int* argc, char*** argv)
  {
    MPI_Init(argc, argv);
    svtkMPIController* controller = svtkMPIController::New();
    controller->Initialize(argc, argv, 1);
    svtkMultiProcessController::SetGlobalController(controller);
  }

  ~Initializer()
  {
    svtkMultiProcessController::GetGlobalController()->Finalize();
    svtkMultiProcessController::GetGlobalController()->Delete();
    svtkMultiProcessController::SetGlobalController(nullptr);
  }
};

bool AllRanksSucceeded(bool status)
{
  svtkMultiProcessController* contr = svtkMultiProcessController::GetGlobalController();
  int success = status ? 1 : 0;
  int allsuccess = 0;
  contr->AllReduce(&success, &allsuccess, 1, svtkCommunicator::MIN_OP);
  return (allsuccess == 1);
}
}

int TestPExtractDataArraysOverTime(int argc, char* argv[])
{
  Initializer init(&argc, &argv);

  svtkMultiProcessController* contr = svtkMultiProcessController::GetGlobalController();
  if (contr == nullptr || contr->GetNumberOfProcesses() != 2)
  {
    cerr << "TestPExtractDataArraysOverTime requires 2 ranks." << endl;
    return EXIT_FAILURE;
  }

  const int myrank = contr->GetLocalProcessId();
  const int numranks = contr->GetNumberOfProcesses();

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/can.ex2");

  svtkNew<svtkPExodusIIReader> reader;
  reader->SetFileName(fname);
  reader->SetController(contr);
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

  svtkNew<svtkPExtractDataArraysOverTime> extractor;
  extractor->SetReportStatisticsOnly(true);
  extractor->SetInputConnection(textracter->GetOutputPort());
  extractor->UpdatePiece(myrank, numranks, 0);

  if (!AllRanksSucceeded(
        ValidateStats(svtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
          num_timesteps, myrank)))
  {
    cerr << "ERROR: Failed to validate dataset at line: " << __LINE__ << endl;
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
  extractor->UpdatePiece(myrank, numranks, 0);
  if (!AllRanksSucceeded(
        ValidateGID(svtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
          num_timesteps, "gid=100", myrank)))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  // this time, simply use element id.
  extractor->SetUseGlobalIDs(false);
  extractor->UpdatePiece(myrank, numranks, 0);
  if (!AllRanksSucceeded(
        ValidateID(svtkMultiBlockDataSet::SafeDownCast(extractor->GetOutputDataObject(0)),
          num_timesteps, "originalId=99 block=2", myrank)))
  {
    cerr << "Failed to validate dataset at line: " << __LINE__ << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
