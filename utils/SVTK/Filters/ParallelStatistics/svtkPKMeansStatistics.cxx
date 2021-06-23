/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPKMeansStatistics.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
#include "svtkToolkits.h"

#include "svtkCommunicator.h"
#include "svtkDoubleArray.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkKMeansDistanceFunctor.h"
#include "svtkKMeansStatistics.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPKMeansStatistics.h"
#include "svtkTable.h"
#include "svtkVariantArray.h"

svtkStandardNewMacro(svtkPKMeansStatistics);
svtkCxxSetObjectMacro(svtkPKMeansStatistics, Controller, svtkMultiProcessController);
//-----------------------------------------------------------------------------
svtkPKMeansStatistics::svtkPKMeansStatistics()
{
  this->Controller = 0;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//-----------------------------------------------------------------------------
svtkPKMeansStatistics::~svtkPKMeansStatistics()
{
  this->SetController(0);
}

//-----------------------------------------------------------------------------
void svtkPKMeansStatistics::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Controller: " << this->Controller << endl;
}

// ----------------------------------------------------------------------
svtkIdType svtkPKMeansStatistics::GetTotalNumberOfObservations(svtkIdType numObservations)
{
  int np = this->Controller->GetNumberOfProcesses();
  if (np < 2)
  {
    return numObservations;
  }
  // Now get ready for parallel calculations
  svtkCommunicator* com = this->Controller->GetCommunicator();
  if (!com)
  {
    svtkGenericWarningMacro("No parallel communicator.");
    return numObservations;
  }

  svtkIdType totalNumObservations;
  com->AllReduce(&numObservations, &totalNumObservations, 1, svtkCommunicator::SUM_OP);
  return totalNumObservations;
}

// ----------------------------------------------------------------------
void svtkPKMeansStatistics::UpdateClusterCenters(svtkTable* newClusterElements,
  svtkTable* curClusterElements, svtkIdTypeArray* numMembershipChanges,
  svtkIdTypeArray* numDataElementsInCluster, svtkDoubleArray* error, svtkIdTypeArray* startRunID,
  svtkIdTypeArray* endRunID, svtkIntArray* computeRun)
{

  int np = this->Controller->GetNumberOfProcesses();
  if (np < 2)
  {
    this->Superclass::UpdateClusterCenters(newClusterElements, curClusterElements,
      numMembershipChanges, numDataElementsInCluster, error, startRunID, endRunID, computeRun);
    return;
  }
  // Now get ready for parallel calculations
  svtkCommunicator* com = this->Controller->GetCommunicator();
  if (!com)
  {
    svtkGenericWarningMacro("No parallel communicator.");
    this->Superclass::UpdateClusterCenters(newClusterElements, curClusterElements,
      numMembershipChanges, numDataElementsInCluster, error, startRunID, endRunID, computeRun);
    return;
  }

  // (All) gather numMembershipChanges
  svtkIdType nm = numMembershipChanges->GetNumberOfTuples();
  svtkIdType nd = numDataElementsInCluster->GetNumberOfTuples();
  svtkIdType totalIntElements = nm + nd;
  svtkIdType* localIntElements = new svtkIdType[totalIntElements];
  svtkIdType* globalIntElements = new svtkIdType[totalIntElements * np];
  svtkIdType* nmPtr = numMembershipChanges->GetPointer(0);
  svtkIdType* ndPtr = numDataElementsInCluster->GetPointer(0);
  memcpy(localIntElements, nmPtr, nm * sizeof(svtkIdType));
  memcpy(localIntElements + nm, ndPtr, nd * sizeof(svtkIdType));
  com->AllGather(localIntElements, globalIntElements, totalIntElements);

  for (svtkIdType runID = 0; runID < nm; runID++)
  {
    if (computeRun->GetValue(runID))
    {
      svtkIdType numChanges = 0;
      for (int j = 0; j < np; j++)
      {
        numChanges += globalIntElements[j * totalIntElements + runID];
      }
      numMembershipChanges->SetValue(runID, numChanges);
    }
  }

  svtkIdType numCols = newClusterElements->GetNumberOfColumns();
  svtkIdType numRows = newClusterElements->GetNumberOfRows();
  svtkIdType numElements = numCols * numRows;

  svtkDoubleArray* totalError = svtkDoubleArray::New();
  totalError->SetNumberOfTuples(numRows);
  totalError->SetNumberOfComponents(1);
  com->AllReduce(error, totalError, svtkCommunicator::SUM_OP);

  for (svtkIdType runID = 0; runID < startRunID->GetNumberOfTuples(); runID++)
  {
    if (computeRun->GetValue(runID))
    {
      for (svtkIdType i = startRunID->GetValue(runID); i < endRunID->GetValue(runID); i++)
      {
        error->SetValue(i, totalError->GetValue(i));
      }
    }
  }
  totalError->Delete();

  svtkTable* allNewClusterElements = svtkTable::New();
  void* localElements = this->DistanceFunctor->AllocateElementArray(numElements);
  void* globalElements = this->DistanceFunctor->AllocateElementArray(numElements * np);
  this->DistanceFunctor->PackElements(newClusterElements, localElements);
  com->AllGatherVoidArray(
    localElements, globalElements, numElements, this->DistanceFunctor->GetDataType());
  this->DistanceFunctor->UnPackElements(
    newClusterElements, allNewClusterElements, localElements, globalElements, np);

  for (svtkIdType runID = 0; runID < startRunID->GetNumberOfTuples(); runID++)
  {
    if (computeRun->GetValue(runID))
    {
      for (svtkIdType i = startRunID->GetValue(runID); i < endRunID->GetValue(runID); i++)
      {
        newClusterElements->SetRow(i, this->DistanceFunctor->GetEmptyTuple(numCols));
        svtkIdType numClusterElements = 0;
        for (int j = 0; j < np; j++)
        {
          numClusterElements += globalIntElements[j * totalIntElements + nm + i];
          this->DistanceFunctor->PairwiseUpdate(newClusterElements, i,
            allNewClusterElements->GetRow(j * numRows + i),
            globalIntElements[j * totalIntElements + nm + i], numClusterElements);
        }
        numDataElementsInCluster->SetValue(i, numClusterElements);

        // check to see if need to perturb
        if (numDataElementsInCluster->GetValue(i) == 0)
        {
          svtkWarningMacro("cluster center " << i - startRunID->GetValue(runID) << " in run "
                                            << runID << " is degenerate. Attempting to perturb");
          this->DistanceFunctor->PerturbElement(newClusterElements, curClusterElements, i,
            startRunID->GetValue(runID), endRunID->GetValue(runID), 0.8);
        }
      }
    }
  }
  delete[] localIntElements;
  delete[] globalIntElements;
  allNewClusterElements->Delete();
}

// ----------------------------------------------------------------------
void svtkPKMeansStatistics::CreateInitialClusterCenters(svtkIdType numToAllocate,
  svtkIdTypeArray* numberOfClusters, svtkTable* inData, svtkTable* curClusterElements,
  svtkTable* newClusterElements)
{

  int np = this->Controller->GetNumberOfProcesses();
  if (np < 2)
  {
    this->Superclass::CreateInitialClusterCenters(
      numToAllocate, numberOfClusters, inData, curClusterElements, newClusterElements);
    return;
  }
  // Now get ready for parallel calculations
  svtkCommunicator* com = this->Controller->GetCommunicator();
  if (!com)
  {
    svtkGenericWarningMacro("No parallel communicator.");
    this->Superclass::CreateInitialClusterCenters(
      numToAllocate, numberOfClusters, inData, curClusterElements, newClusterElements);
    return;
  }

  svtkIdType myRank = com->GetLocalProcessId();
  // use node 0 to broadcast
  svtkIdType broadcastNode = 0;

  // generate data on one node only
  if (myRank == broadcastNode)
  {
    this->Superclass::CreateInitialClusterCenters(
      numToAllocate, numberOfClusters, inData, curClusterElements, newClusterElements);
  }

  int numElements = numToAllocate * curClusterElements->GetNumberOfColumns();
  void* localElements = this->DistanceFunctor->AllocateElementArray(numElements);
  this->DistanceFunctor->PackElements(curClusterElements, localElements);
  if (!com->BroadcastVoidArray(
        localElements, numElements, this->DistanceFunctor->GetDataType(), broadcastNode))
  {
    svtkErrorMacro("Could not broadcast initial cluster coordinates");
    return;
  }

  if (myRank != broadcastNode)
  {
    svtkIdType numCols = curClusterElements->GetNumberOfColumns();
    this->DistanceFunctor->UnPackElements(
      curClusterElements, localElements, numToAllocate, numCols);
    this->DistanceFunctor->UnPackElements(
      newClusterElements, localElements, numToAllocate, numCols);
    for (svtkIdType i = 0; i < numToAllocate; i++)
    {
      numberOfClusters->InsertNextValue(numToAllocate);
    }
  }

  this->DistanceFunctor->DeallocateElementArray(localElements);
}
