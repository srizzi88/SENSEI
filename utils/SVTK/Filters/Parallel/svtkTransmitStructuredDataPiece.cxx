/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransmitStructuredDataPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTransmitStructuredDataPiece.h"

#include "svtkDataSet.h"
#include "svtkExtentTranslator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkTransmitStructuredDataPiece);

svtkCxxSetObjectMacro(svtkTransmitStructuredDataPiece, Controller, svtkMultiProcessController);

//----------------------------------------------------------------------------
svtkTransmitStructuredDataPiece::svtkTransmitStructuredDataPiece()
{
  this->Controller = nullptr;
  this->CreateGhostCells = 1;
  this->SetNumberOfInputPorts(1);
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkTransmitStructuredDataPiece::~svtkTransmitStructuredDataPiece()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int svtkTransmitStructuredDataPiece::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->Controller)
  {
    int wExt[6];
    if (this->Controller->GetLocalProcessId() == 0)
    {
      svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
      inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExt);
    }
    this->Controller->Broadcast(wExt, 6, 0);
    svtkInformation* outInfo = outputVector->GetInformationObject(0);
    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wExt, 6);
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkTransmitStructuredDataPiece::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  if (this->Controller)
  {
    if (this->Controller->GetLocalProcessId() > 0)
    {
      int wExt[6] = { 0, -1, 0, -1, 0, -1 };
      inputVector[0]->GetInformationObject(0)->Set(
        svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), wExt, 6);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkTransmitStructuredDataPiece::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataSet* output = svtkDataSet::GetData(outputVector);

  int procId;

  if (this->Controller == nullptr)
  {
    svtkErrorMacro("Could not find Controller.");
    return 1;
  }

  procId = this->Controller->GetLocalProcessId();
  if (procId == 0)
  {
    svtkDataSet* input = svtkDataSet::GetData(inputVector[0]);
    this->RootExecute(input, output, outInfo);
  }
  else
  {
    this->SatelliteExecute(procId, output, outInfo);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkTransmitStructuredDataPiece::RootExecute(
  svtkDataSet* input, svtkDataSet* output, svtkInformation* outInfo)
{
  svtkDataSet* tmp = input->NewInstance();
  int numProcs, i;

  int updatePiece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int updateNumPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int updatedGhost =
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  if (!this->CreateGhostCells)
  {
    updatedGhost = 0;
  }
  int* wholeExt = input->GetInformation()->Get(svtkDataObject::DATA_EXTENT());

  svtkExtentTranslator* et = svtkExtentTranslator::New();

  int newExt[6];
  et->PieceToExtentThreadSafe(updatePiece, updateNumPieces, updatedGhost, wholeExt, newExt,
    svtkExtentTranslator::BLOCK_MODE, 0);
  output->ShallowCopy(input);
  output->Crop(newExt);

  if (updatedGhost > 0)
  {
    // Create ghost array
    int zeroExt[6];
    et->PieceToExtentThreadSafe(
      updatePiece, updateNumPieces, 0, wholeExt, zeroExt, svtkExtentTranslator::BLOCK_MODE, 0);
    output->GenerateGhostArray(zeroExt);
  }

  numProcs = this->Controller->GetNumberOfProcesses();
  for (i = 1; i < numProcs; ++i)
  {
    int updateInfo[3];
    this->Controller->Receive(updateInfo, 3, i, 22341);
    et->PieceToExtentThreadSafe(updateInfo[0], updateInfo[1], updateInfo[2], wholeExt, newExt,
      svtkExtentTranslator::BLOCK_MODE, 0);
    tmp->ShallowCopy(input);
    tmp->Crop(newExt);

    if (updateInfo[2] > 0)
    {
      // Create ghost array
      int zeroExt[6];
      et->PieceToExtentThreadSafe(
        updateInfo[0], updateInfo[1], 0, wholeExt, zeroExt, svtkExtentTranslator::BLOCK_MODE, 0);
      tmp->GenerateGhostArray(zeroExt);
    }

    this->Controller->Send(tmp, i, 22342);
  }

  // clean up the structures we've used here
  tmp->Delete();
  et->Delete();
}

//----------------------------------------------------------------------------
void svtkTransmitStructuredDataPiece::SatelliteExecute(
  int, svtkDataSet* output, svtkInformation* outInfo)
{
  int updatePiece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int updateNumPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int updatedGhost =
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  if (!this->CreateGhostCells)
  {
    updatedGhost = 0;
  }

  int updateInfo[3];
  updateInfo[0] = updatePiece;
  updateInfo[1] = updateNumPieces;
  updateInfo[2] = updatedGhost;

  this->Controller->Send(updateInfo, 3, 0, 22341);

  // receive root's response
  this->Controller->Receive(output, 0, 22342);
}

//----------------------------------------------------------------------------
void svtkTransmitStructuredDataPiece::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");

  os << indent << "Controller: (" << this->Controller << ")\n";
}
