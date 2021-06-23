/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPieceRequestFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPieceRequestFilter.h"

#include "svtkDataObject.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkPieceRequestFilter);

//----------------------------------------------------------------------------
svtkPieceRequestFilter::svtkPieceRequestFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->Piece = 0;
  this->NumberOfPieces = 1;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkPieceRequestFilter::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkPieceRequestFilter::GetOutput(int port)
{
  return svtkDataObject::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void svtkPieceRequestFilter::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkPieceRequestFilter::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkPieceRequestFilter::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  // create the output
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  // set update extent
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkPieceRequestFilter::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      svtkInformation* info = outputVector->GetInformationObject(i);
      svtkDataObject* output = info->Get(svtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        svtkDataObject* newOutput = input->NewInstance();
        info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkPieceRequestFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDataObject* input = svtkDataObject::GetData(inputVector[0]);
  svtkDataObject* output = svtkDataObject::GetData(outputVector);
  output->ShallowCopy(input);
  return 1;
}

//----------------------------------------------------------------------------
int svtkPieceRequestFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), this->NumberOfPieces);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), this->Piece);
  return 1;
}

//----------------------------------------------------------------------------
int svtkPieceRequestFilter::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int svtkPieceRequestFilter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void svtkPieceRequestFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << endl;
  os << indent << "Piece: " << this->Piece << endl;
}
