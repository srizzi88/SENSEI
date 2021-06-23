/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransmitUnstructuredGridPiece.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTransmitUnstructuredGridPiece.h"

#include "svtkCellData.h"
#include "svtkExtractUnstructuredGridPiece.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkTransmitUnstructuredGridPiece);

svtkCxxSetObjectMacro(svtkTransmitUnstructuredGridPiece, Controller, svtkMultiProcessController);

//----------------------------------------------------------------------------
svtkTransmitUnstructuredGridPiece::svtkTransmitUnstructuredGridPiece()
{
  this->CreateGhostCells = 1;

  // Controller keeps a reference to this object as well.
  this->Controller = nullptr;
  this->SetController(svtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
svtkTransmitUnstructuredGridPiece::~svtkTransmitUnstructuredGridPiece()
{
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int svtkTransmitUnstructuredGridPiece::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkUnstructuredGrid* input =
    svtkUnstructuredGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int procId;

  if (this->Controller == nullptr)
  {
    svtkErrorMacro("Could not find Controller.");
    return 1;
  }

  procId = this->Controller->GetLocalProcessId();
  if (procId == 0)
  {
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
void svtkTransmitUnstructuredGridPiece::RootExecute(
  svtkUnstructuredGrid* input, svtkUnstructuredGrid* output, svtkInformation* outInfo)
{
  svtkUnstructuredGrid* tmp = svtkUnstructuredGrid::New();
  svtkExtractUnstructuredGridPiece* extract = svtkExtractUnstructuredGridPiece::New();
  int ext[3];
  int numProcs, i;

  int outPiece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  if (outPiece != 0)
  {
    svtkWarningMacro(<< "Piece " << outPiece << " does not match process 0.  "
                    << "Altering request to try to avoid a deadlock.");
  }

  svtkStreamingDemandDrivenPipeline* extractExecutive =
    svtkStreamingDemandDrivenPipeline::SafeDownCast(extract->GetExecutive());

  // First, set up the pipeline and handle local request.
  tmp->ShallowCopy(input);
  extract->SetCreateGhostCells(this->CreateGhostCells);
  extract->SetInputData(tmp);
  extractExecutive->UpdateDataObject();

  svtkInformation* extractOutInfo = extractExecutive->GetOutputInformation(0);
  extractOutInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  extractOutInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  extractOutInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  extractOutInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT_INITIALIZED(), 1);

  extract->Update();

  // Copy geometry without copying information.
  output->CopyStructure(extract->GetOutput());
  output->GetPointData()->PassData(extract->GetOutput()->GetPointData());
  output->GetCellData()->PassData(extract->GetOutput()->GetCellData());
  svtkFieldData* inFd = extract->GetOutput()->GetFieldData();
  svtkFieldData* outFd = output->GetFieldData();
  if (inFd && outFd)
  {
    outFd->PassData(inFd);
  }

  // Now do each of the satellite requests.
  numProcs = this->Controller->GetNumberOfProcesses();
  for (i = 1; i < numProcs; ++i)
  {
    this->Controller->Receive(ext, 3, i, 22341);
    extractOutInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), ext[1]);
    extractOutInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), ext[0]);
    extractOutInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ext[2]);
    extract->Update();
    this->Controller->Send(extract->GetOutput(), i, 22342);
  }
  tmp->Delete();
  extract->Delete();
}

//----------------------------------------------------------------------------
void svtkTransmitUnstructuredGridPiece::SatelliteExecute(
  int, svtkUnstructuredGrid* output, svtkInformation* outInfo)
{
  svtkUnstructuredGrid* tmp = svtkUnstructuredGrid::New();
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

  tmp->Delete();
}

//----------------------------------------------------------------------------
void svtkTransmitUnstructuredGridPiece::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Create Ghost Cells: " << (this->CreateGhostCells ? "On\n" : "Off\n");

  os << indent << "Controller: (" << this->Controller << ")\n";
}
