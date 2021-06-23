/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRUtilities.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkParallelAMRUtilities.h"
#include "svtkAMRBox.h"
#include "svtkAMRInformation.h"
#include "svtkCompositeDataIterator.h"
#include "svtkMultiProcessController.h"
#include "svtkOverlappingAMR.h"
#include "svtkSmartPointer.h"
#include "svtkUniformGrid.h"
#include <cassert>
#include <cmath>
#include <limits>

//------------------------------------------------------------------------------
void svtkParallelAMRUtilities::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkParallelAMRUtilities::DistributeProcessInformation(
  svtkOverlappingAMR* amr, svtkMultiProcessController* controller, std::vector<int>& processMap)
{
  processMap.resize(amr->GetTotalNumberOfBlocks(), -1);
  svtkSmartPointer<svtkCompositeDataIterator> iter;
  iter.TakeReference(amr->NewIterator());
  iter->SkipEmptyNodesOn();

  if (!controller || controller->GetNumberOfProcesses() == 1)
  {
    for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      unsigned int index = iter->GetCurrentFlatIndex();
      processMap[index] = 0;
    }
    return;
  }
  svtkAMRInformation* amrInfo = amr->GetAMRInfo();
  int myRank = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  // get the active process ids
  std::vector<int> myBlocks;
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    myBlocks.push_back(iter->GetCurrentFlatIndex());
  }

  svtkIdType myNumBlocks = static_cast<svtkIdType>(myBlocks.size());
  std::vector<svtkIdType> numBlocks(numProcs, 0);
  numBlocks[myRank] = myNumBlocks;

  // gather the active process counts
  controller->AllGather(&myNumBlocks, &numBlocks[0], 1);

  // gather the blocks each process owns into one array
  std::vector<svtkIdType> offsets(numProcs, 0);
  svtkIdType currentOffset(0);
  for (int i = 0; i < numProcs; i++)
  {
    offsets[i] = currentOffset;
    currentOffset += numBlocks[i];
  }
  cout << "(" << myRank << ")"
       << "total # of active blocks: " << currentOffset << " out of total "
       << amrInfo->GetTotalNumberOfBlocks() << endl;
  std::vector<int> allBlocks(currentOffset, -1);
  controller->AllGatherV(
    &myBlocks[0], &allBlocks[0], (svtkIdType)myBlocks.size(), &numBlocks[0], &offsets[0]);

#ifdef DEBUG
  if (myRank == 0)
  {
    for (int i = 0; i < numProcs; i++)
    {
      svtkIdType offset = offsets[i];
      int n = numBlocks[i];
      cout << "Rank " << i << " has: ";
      for (svtkIdType j = offset; j < offset + n; j++)
      {
        cout << allBlocks[j] << " ";
      }
      cout << endl;
    }
  }
#endif
  for (int rank = 0; rank < numProcs; rank++)
  {
    int offset = offsets[rank];
    int n = numBlocks[rank];
    for (int j = offset; j < offset + n; j++)
    {
      int index = allBlocks[j];
      assert(index >= 0);
      processMap[index] = rank;
    }
  }
}

//------------------------------------------------------------------------------
void svtkParallelAMRUtilities::StripGhostLayers(svtkOverlappingAMR* ghostedAMRData,
  svtkOverlappingAMR* strippedAMRData, svtkMultiProcessController* controller)
{
  svtkAMRUtilities::StripGhostLayers(ghostedAMRData, strippedAMRData);

  if (controller != nullptr)
  {
    controller->Barrier();
  }
}

//------------------------------------------------------------------------------
void svtkParallelAMRUtilities::BlankCells(
  svtkOverlappingAMR* amr, svtkMultiProcessController* myController)
{
  svtkAMRInformation* info = amr->GetAMRInfo();
  if (!info->HasRefinementRatio())
  {
    info->GenerateRefinementRatio();
  }
  if (!info->HasChildrenInformation())
  {
    info->GenerateParentChildInformation();
  }

  std::vector<int> processorMap;
  svtkParallelAMRUtilities::DistributeProcessInformation(amr, myController, processorMap);
  unsigned int numLevels = info->GetNumberOfLevels();
  for (unsigned int i = 0; i < numLevels; i++)
  {
    svtkAMRUtilities::BlankGridsAtLevel(amr, i, info->GetChildrenAtLevel(i), processorMap);
  }
}
