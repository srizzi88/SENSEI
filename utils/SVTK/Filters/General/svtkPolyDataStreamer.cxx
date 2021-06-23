/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataStreamer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyDataStreamer.h"

#include "svtkAppendPolyData.h"
#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkPolyDataStreamer);

//----------------------------------------------------------------------------
svtkPolyDataStreamer::svtkPolyDataStreamer()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->NumberOfPasses = 2;
  this->ColorByPiece = 0;

  this->Append = svtkAppendPolyData::New();
}

//----------------------------------------------------------------------------
svtkPolyDataStreamer::~svtkPolyDataStreamer()
{
  this->Append->Delete();
  this->Append = nullptr;
}

//----------------------------------------------------------------------------
void svtkPolyDataStreamer::SetNumberOfStreamDivisions(int num)
{
  if (this->NumberOfPasses == (unsigned int)num)
  {
    return;
  }

  this->Modified();
  this->NumberOfPasses = num;
}

//----------------------------------------------------------------------------
int svtkPolyDataStreamer::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int outPiece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int outNumPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outPiece * this->NumberOfPasses + this->CurrentIndex);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outNumPieces * this->NumberOfPasses);

  return 1;
}

//----------------------------------------------------------------------------
int svtkPolyDataStreamer::ExecutePass(
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPolyData* copy = svtkPolyData::New();
  copy->ShallowCopy(input);
  this->Append->AddInputData(copy);

  if (this->ColorByPiece)
  {
    int inPiece = inInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    svtkFloatArray* pieceColors = svtkFloatArray::New();
    pieceColors->SetName("Piece Colors");
    svtkIdType numCells = input->GetNumberOfCells();
    pieceColors->SetNumberOfTuples(numCells);
    for (svtkIdType j = 0; j < numCells; ++j)
    {
      pieceColors->SetValue(j, inPiece);
    }
    int idx = copy->GetCellData()->AddArray(pieceColors);
    copy->GetCellData()->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    pieceColors->Delete();
  }

  copy->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int svtkPolyDataStreamer::PostExecute(
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  this->Append->Update();
  output->ShallowCopy(this->Append->GetOutput());
  this->Append->RemoveAllInputConnections(0);
  this->Append->GetOutput()->Initialize();

  return 1;
}

//----------------------------------------------------------------------------
void svtkPolyDataStreamer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "NumberOfStreamDivisions: " << this->NumberOfPasses << endl;
  os << indent << "ColorByPiece: " << this->ColorByPiece << endl;
}

//----------------------------------------------------------------------------
int svtkPolyDataStreamer::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPolyData");
  return 1;
}

//----------------------------------------------------------------------------
int svtkPolyDataStreamer::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}
