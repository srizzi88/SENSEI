/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCollectTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "svtkCollectTable.h"

#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkSocketController.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"
#include "svtkVariant.h"

svtkStandardNewMacro(svtkCollectTable);

svtkCxxSetObjectMacro(svtkCollectTable, Controller, svtkMultiProcessController);
svtkCxxSetObjectMacro(svtkCollectTable, SocketController, svtkSocketController);

//----------------------------------------------------------------------------
svtkCollectTable::svtkCollectTable()
{
  this->PassThrough = 0;
  this->SocketController = nullptr;

  // Controller keeps a reference to this object as well.
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkCollectTable::~svtkCollectTable()
{
  this->SetController(nullptr);
  this->SetSocketController(nullptr);
}

//--------------------------------------------------------------------------
int svtkCollectTable::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
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
int svtkCollectTable::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkTable* input = svtkTable::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTable* output = svtkTable::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int numProcs, myId;
  int idx;

  if (this->Controller == nullptr && this->SocketController == nullptr)
  { // Running as a single process.
    output->ShallowCopy(input);
    return 1;
  }

  if (this->Controller == nullptr && this->SocketController != nullptr)
  { // This is a client.  We assume no data on client for input.
    if (!this->PassThrough)
    {
      svtkTable* table = svtkTable::New();
      this->SocketController->Receive(table, 1, 121767);
      output->ShallowCopy(table);
      table->Delete();
      table = nullptr;
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
    output->ShallowCopy(input);
    return 1;
  }

  // Collect.
  if (myId == 0)
  {
    svtkTable* wholeTable = svtkTable::New();
    wholeTable->ShallowCopy(input);

    for (idx = 1; idx < numProcs; ++idx)
    {
      svtkTable* curTable = svtkTable::New();
      this->Controller->Receive(curTable, idx, 121767);
      svtkIdType numRows = curTable->GetNumberOfRows();
      svtkIdType numCols = curTable->GetNumberOfColumns();
      for (svtkIdType i = 0; i < numRows; i++)
      {
        svtkIdType curRow = wholeTable->InsertNextBlankRow();
        for (svtkIdType j = 0; j < numCols; j++)
        {
          wholeTable->SetValue(curRow, j, curTable->GetValue(i, j));
        }
      }
      curTable->Delete();
    }

    if (this->SocketController)
    { // Send collected data onto client.
      this->SocketController->Send(wholeTable, 1, 121767);
      // output will be empty.
    }
    else
    { // No client. Keep the output here.
      output->ShallowCopy(wholeTable);
    }
  }
  else
  {
    this->Controller->Send(input, 0, 121767);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkCollectTable::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PassThough: " << this->PassThrough << endl;
  os << indent << "Controller: (" << this->Controller << ")\n";
  os << indent << "SocketController: (" << this->SocketController << ")\n";
}
