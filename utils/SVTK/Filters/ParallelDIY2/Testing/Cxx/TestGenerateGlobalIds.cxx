/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGenerateGlobalIds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

===========================================================================*/

#include "svtkCellData.h"
#include "svtkCommunicator.h"
#include "svtkDataSet.h"
#include "svtkExtentTranslator.h"
#include "svtkGenerateGlobalIds.h"
#include "svtkIdTypeArray.h"
#include "svtkLogger.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkRTAnalyticSource.h"
#include "svtkStructuredData.h"
#include "svtkUnsignedCharArray.h"

#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
#include "svtkMPIController.h"
#else
#include "svtkDummyController.h"
#endif

namespace
{
static int whole_extent[] = { 0, 99, 0, 99, 0, 99 };

svtkSmartPointer<svtkMultiBlockDataSet> CreateDataSet(
  svtkMultiProcessController* contr, int ghost_level, int nblocks)
{
  const int num_ranks = contr ? contr->GetNumberOfProcesses() : 1;
  const int rank = contr ? contr->GetLocalProcessId() : 0;
  const int total_nblocks = nblocks * num_ranks;

  svtkNew<svtkExtentTranslator> translator;
  translator->SetWholeExtent(whole_extent);
  translator->SetNumberOfPieces(total_nblocks);
  translator->SetGhostLevel(ghost_level);

  svtkNew<svtkMultiBlockDataSet> mb;
  for (int cc = 0; cc < nblocks; ++cc)
  {
    translator->SetPiece(rank * nblocks + cc);
    translator->PieceToExtent();

    int ext[6];
    translator->GetExtent(ext);

    svtkNew<svtkRTAnalyticSource> source;
    source->SetWholeExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
    source->Update();

    mb->SetBlock(rank * nblocks + cc, source->GetOutputDataObject(0));
  }
  return mb;
}

bool ValidateDataset(svtkMultiBlockDataSet* mb, svtkMultiProcessController* contr,
  int svtkNotUsed(ghost_level), int nblocks)
{
  const int num_ranks = contr ? contr->GetNumberOfProcesses() : 1;
  const int total_nblocks = nblocks * num_ranks;

  svtkIdType local_non_duplicated_points = 0;
  svtkIdType local_ptid_max = 0;
  svtkIdType local_cellid_max = 0;
  for (int cc = 0; cc < total_nblocks; ++cc)
  {
    if (auto ds = svtkDataSet::SafeDownCast(mb->GetBlock(cc)))
    {
      if (auto gpoints = svtkUnsignedCharArray::SafeDownCast(
            ds->GetPointData()->GetArray(svtkDataSetAttributes::GhostArrayName())))
      {
        for (svtkIdType kk = 0, max = gpoints->GetNumberOfTuples(); kk < max; ++kk)
        {
          local_non_duplicated_points += (gpoints->GetTypedComponent(kk, 0) == 0) ? 1 : 0;
        }
      }
      if (auto gpids = svtkIdTypeArray::SafeDownCast(ds->GetPointData()->GetGlobalIds()))
      {
        local_ptid_max = std::max(static_cast<svtkIdType>(gpids->GetRange(0)[1]), local_ptid_max);
      }
      if (auto gcids = svtkIdTypeArray::SafeDownCast(ds->GetCellData()->GetGlobalIds()))
      {
        local_cellid_max =
          std::max(static_cast<svtkIdType>(gcids->GetRange(0)[1]), local_cellid_max);
      }
    }
  }

  svtkIdType global_non_duplicated_points;
  contr->AllReduce(
    &local_non_duplicated_points, &global_non_duplicated_points, 1, svtkCommunicator::SUM_OP);
  if (global_non_duplicated_points != svtkStructuredData::GetNumberOfPoints(whole_extent))
  {
    svtkLogF(ERROR, "incorrect non-duplicated points; ghost points marked incorrectly!");
    return false;
  }

  svtkIdType global_ptid_max;
  contr->AllReduce(&local_ptid_max, &global_ptid_max, 1, svtkCommunicator::MAX_OP);
  const svtkIdType expected_ptid_max = svtkStructuredData::GetNumberOfPoints(whole_extent) - 1;
  if (global_ptid_max != expected_ptid_max)
  {
    svtkLogF(
      ERROR, "incorrect global point ids! %d, %d", (int)global_ptid_max, (int)expected_ptid_max);
    return false;
  }

  svtkIdType global_cellid_max;
  contr->AllReduce(&local_cellid_max, &global_cellid_max, 1, svtkCommunicator::MAX_OP);
  if (global_cellid_max != svtkStructuredData::GetNumberOfCells(whole_extent) - 1)
  {
    svtkLogF(ERROR, "incorrect global cell ids!");
    return false;
  }

  return true;
}

}

int TestGenerateGlobalIds(int argc, char* argv[])
{
#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
  svtkMPIController* contr = svtkMPIController::New();
#else
  svtkDummyController* contr = svtkDummyController::New();
#endif
  contr->Initialize(&argc, &argv);
  svtkMultiProcessController::SetGlobalController(contr);

  int status = EXIT_SUCCESS;
  if (auto dataset = CreateDataSet(contr, 0, 3)) // no cell overlap
  {
    svtkNew<svtkGenerateGlobalIds> generator;
    generator->SetInputDataObject(dataset);
    generator->Update();

    if (!ValidateDataset(
          svtkMultiBlockDataSet::SafeDownCast(generator->GetOutputDataObject(0)), contr, 0, 3))
    {
      status = EXIT_FAILURE;
    }
  }

  if (auto dataset = CreateDataSet(contr, 1, 3)) // cell overlap
  {
    svtkNew<svtkGenerateGlobalIds> generator;
    generator->SetInputDataObject(dataset);
    generator->Update();

    if (!ValidateDataset(
          svtkMultiBlockDataSet::SafeDownCast(generator->GetOutputDataObject(0)), contr, 1, 3))
    {
      status = EXIT_FAILURE;
    }
  }

  // test a dataset with 1 block per rank.
  if (auto dataset = CreateDataSet(contr, 1, 1))
  {
    svtkNew<svtkGenerateGlobalIds> generator;
    generator->SetInputDataObject(dataset);
    generator->Update();

    if (!ValidateDataset(
          svtkMultiBlockDataSet::SafeDownCast(generator->GetOutputDataObject(0)), contr, 1, 1))
    {
      status = EXIT_FAILURE;
    }
  }

  svtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  contr->Delete();
  return status;
}
