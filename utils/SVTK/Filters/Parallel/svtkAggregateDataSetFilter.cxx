/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAggregateDataSetFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAggregateDataSetFilter.h"

#include "svtkAppendFilter.h"
#include "svtkAppendPolyData.h"
#include "svtkCommunicator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnstructuredGrid.h"

svtkObjectFactoryNewMacro(svtkAggregateDataSetFilter);

//-----------------------------------------------------------------------------
svtkAggregateDataSetFilter::svtkAggregateDataSetFilter()
{
  this->NumberOfTargetProcesses = 1;
}

//-----------------------------------------------------------------------------
svtkAggregateDataSetFilter::~svtkAggregateDataSetFilter() = default;

//-----------------------------------------------------------------------------
void svtkAggregateDataSetFilter::SetNumberOfTargetProcesses(int tp)
{
  if (tp != this->NumberOfTargetProcesses)
  {
    int numProcs = svtkMultiProcessController::GetGlobalController()->GetNumberOfProcesses();
    if (tp > 0 && tp <= numProcs)
    {
      this->NumberOfTargetProcesses = tp;
      this->Modified();
    }
    else if (tp <= 0 && this->NumberOfTargetProcesses != 1)
    {
      this->NumberOfTargetProcesses = 1;
      this->Modified();
    }
    else if (tp > numProcs && this->NumberOfTargetProcesses != numProcs)
    {
      this->NumberOfTargetProcesses = numProcs;
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
int svtkAggregateDataSetFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
// We should avoid marshalling more than once.
int svtkAggregateDataSetFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDataSet* input = nullptr;
  svtkDataSet* output = svtkDataSet::GetData(outputVector, 0);

  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
  {
    input = svtkDataSet::GetData(inputVector[0], 0);
  }

  svtkMultiProcessController* controller = svtkMultiProcessController::GetGlobalController();

  int numberOfProcesses = controller->GetNumberOfProcesses();
  if (numberOfProcesses == this->NumberOfTargetProcesses)
  {
    if (input)
    {
      output->ShallowCopy(input);
    }
    return 1;
  }

  if (input->IsA("svtkImageData") || input->IsA("svtkRectilinearGrid") ||
    input->IsA("svtkStructuredGrid"))
  {
    svtkErrorMacro("Must build with the svtkFiltersParallelDIY2 module enabled to "
      << "aggregate topologically regular grids with MPI");

    return 0;
  }

  // create a subcontroller to simplify communication between the processes
  // that are aggregating data
  svtkSmartPointer<svtkMultiProcessController> subController = nullptr;
  if (this->NumberOfTargetProcesses == 1)
  {
    subController = controller;
  }
  else
  {
    int localProcessId = controller->GetLocalProcessId();
    int numberOfProcessesPerGroup = numberOfProcesses / this->NumberOfTargetProcesses;
    int localColor = localProcessId / numberOfProcessesPerGroup;
    if (numberOfProcesses % this->NumberOfTargetProcesses)
    {
      double d = 1. * numberOfProcesses / this->NumberOfTargetProcesses;
      localColor = int(localProcessId / d);
    }
    subController.TakeReference(controller->PartitionController(localColor, 0));
  }

  int subNumProcs = subController->GetNumberOfProcesses();
  int subRank = subController->GetLocalProcessId();

  std::vector<svtkIdType> pointCount(subNumProcs, 0);
  svtkIdType numPoints = input->GetNumberOfPoints();
  subController->AllGather(&numPoints, &pointCount[0], 1);

  // The first process in the subcontroller to have points is the one that data will
  // be aggregated to. All of the other processes send their data set to that process.
  int receiveProc = 0;
  svtkIdType maxVal = 0;
  for (int i = 0; i < subNumProcs; i++)
  {
    if (pointCount[i] > maxVal)
    {
      maxVal = pointCount[i];
      receiveProc = i;
    }
  }

  std::vector<svtkSmartPointer<svtkDataObject> > recvBuffer;
  subController->Gather(input, recvBuffer, receiveProc);
  if (subRank == receiveProc)
  {
    if (recvBuffer.size() == 1)
    {
      output->ShallowCopy(input);
    }
    else if (input->IsA("svtkPolyData"))
    {
      svtkNew<svtkAppendPolyData> appendFilter;
      for (std::vector<svtkSmartPointer<svtkDataObject> >::iterator it = recvBuffer.begin();
           it != recvBuffer.end(); ++it)
      {
        appendFilter->AddInputData(svtkPolyData::SafeDownCast(*it));
      }
      appendFilter->Update();
      output->ShallowCopy(appendFilter->GetOutput());
    }
    else if (input->IsA("svtkUnstructuredGrid"))
    {
      svtkNew<svtkAppendFilter> appendFilter;
      appendFilter->MergePointsOn();
      for (std::vector<svtkSmartPointer<svtkDataObject> >::iterator it = recvBuffer.begin();
           it != recvBuffer.end(); ++it)
      {
        appendFilter->AddInputData(*it);
      }
      appendFilter->Update();
      output->ShallowCopy(appendFilter->GetOutput());
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkAggregateDataSetFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfTargetProcesses: " << this->NumberOfTargetProcesses << endl;
}
