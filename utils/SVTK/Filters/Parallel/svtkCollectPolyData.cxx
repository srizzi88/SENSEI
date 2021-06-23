/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollectPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCollectPolyData.h"

#include "svtkAppendPolyData.h"
#include "svtkCellData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSocketController.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkCollectPolyData);

svtkCxxSetObjectMacro(svtkCollectPolyData, Controller, svtkMultiProcessController);
svtkCxxSetObjectMacro(svtkCollectPolyData, SocketController, svtkSocketController);

//----------------------------------------------------------------------------
svtkCollectPolyData::svtkCollectPolyData()
{
  this->PassThrough = 0;
  this->SocketController = nullptr;

  // Controller keeps a reference to this object as well.
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkCollectPolyData::~svtkCollectPolyData()
{
  this->SetController(nullptr);
  this->SetSocketController(nullptr);
}

//--------------------------------------------------------------------------
int svtkCollectPolyData::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));

  return 1;
}

//----------------------------------------------------------------------------
int svtkCollectPolyData::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int numProcs, myId;
  int idx;

  if (this->Controller == nullptr && this->SocketController == nullptr)
  { // Running as a single process.
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return 1;
  }

  if (this->Controller == nullptr && this->SocketController != nullptr)
  { // This is a client.  We assume no data on client for input.
    if (!this->PassThrough)
    {
      svtkPolyData* pd = svtkPolyData::New();
      this->SocketController->Receive(pd, 1, 121767);
      output->CopyStructure(pd);
      output->GetPointData()->PassData(pd->GetPointData());
      output->GetCellData()->PassData(pd->GetCellData());
      pd->Delete();
      pd = nullptr;
      return 1;
    }
    // If not collected, output will be empty from initialization.
    return 0;
  }

  myId = this->Controller->GetLocalProcessId();
  numProcs = this->Controller->GetNumberOfProcesses();

  if (this->PassThrough)
  {
    // Just copy and return (no collection).
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());
    return 1;
  }

  // Collect.
  svtkAppendPolyData* append = svtkAppendPolyData::New();
  svtkPolyData* pd = nullptr;

  if (myId == 0)
  {
    pd = svtkPolyData::New();
    pd->CopyStructure(input);
    pd->GetPointData()->PassData(input->GetPointData());
    pd->GetCellData()->PassData(input->GetCellData());
    append->AddInputData(pd);
    pd->Delete();
    for (idx = 1; idx < numProcs; ++idx)
    {
      pd = svtkPolyData::New();
      this->Controller->Receive(pd, idx, 121767);
      append->AddInputData(pd);
      pd->Delete();
      pd = nullptr;
    }
    append->Update();
    input = append->GetOutput();
    if (this->SocketController)
    { // Send collected data onto client.
      this->SocketController->Send(input, 1, 121767);
      // output will be empty.
    }
    else
    { // No client. Keep the output here.
      output->CopyStructure(input);
      output->GetPointData()->PassData(input->GetPointData());
      output->GetCellData()->PassData(input->GetCellData());
    }
    append->Delete();
    append = nullptr;
  }
  else
  {
    this->Controller->Send(input, 0, 121767);
    append->Delete();
    append = nullptr;
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkCollectPolyData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PassThough: " << this->PassThrough << endl;
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "SocketController: (" << this->SocketController << ")\n";
}
