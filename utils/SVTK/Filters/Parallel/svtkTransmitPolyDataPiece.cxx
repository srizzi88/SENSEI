/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransmitPolyDataPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTransmitPolyDataPiece.h"

#include "svtkCellData.h"
#include "svtkExtractPolyDataPiece.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkTransmitPolyDataPiece);

svtkCxxSetObjectMacro(svtkTransmitPolyDataPiece, Controller, svtkMultiProcessController);

//----------------------------------------------------------------------------
svtkTransmitPolyDataPiece::svtkTransmitPolyDataPiece()
{
  this->CreateGhostCells = 1;

  // Controller keeps a reference to this object as well.
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkTransmitPolyDataPiece::~svtkTransmitPolyDataPiece()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int svtkTransmitPolyDataPiece::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int procId;
  if (this->Controller == nullptr)
  {
    svtkErrorMacro("Could not find Controller.");
    return 0;
  }

  procId = this->Controller->GetLocalProcessId();
  if (procId == 0)
  {
    // It is important to synchronize these calls (all processes execute)
    // cerr << "Root Execute\n";
    this->RootExecute(input, output, outInfo);
  }
  else
  {
    // cerr << "Satellite Execute " << procId << endl;
    this->SatelliteExecute(procId, output, outInfo);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkTransmitPolyDataPiece::RootExecute(
  svtkPolyData* input, svtkPolyData* output, svtkInformation* outInfo)
{
  svtkPolyData* tmp = svtkPolyData::New();
  svtkExtractPolyDataPiece* extract = svtkExtractPolyDataPiece::New();
  int ext[3];
  int numProcs, i;

  // First, set up the pipeline and handle local request.
  tmp->ShallowCopy(input);
  extract->SetCreateGhostCells(this->CreateGhostCells);
  extract->SetInputData(tmp);

  int nPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int ghosts = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
  extract->UpdatePiece(piece, nPieces, ghosts);

  // Copy geometry without copying information.
  output->CopyStructure(extract->GetOutput());
  output->GetPointData()->PassData(extract->GetOutput()->GetPointData());
  output->GetCellData()->PassData(extract->GetOutput()->GetCellData());
  output->GetFieldData()->PassData(extract->GetOutput()->GetFieldData());

  // Now do each of the satellite requests.
  numProcs = this->Controller->GetNumberOfProcesses();
  for (i = 1; i < numProcs; ++i)
  {
    this->Controller->Receive(ext, 3, i, 22341);
    extract->UpdatePiece(ext[0], ext[1], ext[2]);
    this->Controller->Send(extract->GetOutput(), i, 22342);
  }
  tmp->Delete();
  extract->Delete();
}

//----------------------------------------------------------------------------
void svtkTransmitPolyDataPiece::SatelliteExecute(int, svtkPolyData* output, svtkInformation* outInfo)
{
  svtkPolyData* tmp = svtkPolyData::New();
  int ext[3];

  ext[0] = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  ext[1] = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ext[2] = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  this->Controller->Send(ext, 3, 0, 22341);
  this->Controller->Receive(tmp, 0, 22342);

  // Copy geometry without copying information.
  output->CopyStructure(tmp);
  output->GetPointData()->PassData(tmp->GetPointData());
  output->GetCellData()->PassData(tmp->GetCellData());
  output->GetFieldData()->PassData(tmp->GetFieldData());

  tmp->Delete();
}

//----------------------------------------------------------------------------
void svtkTransmitPolyDataPiece::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");

  os << indent << "Controller: (" << this->Controller << ")\n";
}
