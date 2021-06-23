/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataObjectAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataObjectAlgorithm.h"

#include "svtkCommand.h"
#include "svtkDataObject.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkDataObjectAlgorithm);

//----------------------------------------------------------------------------
svtkDataObjectAlgorithm::svtkDataObjectAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkDataObjectAlgorithm::~svtkDataObjectAlgorithm() = default;

//----------------------------------------------------------------------------
void svtkDataObjectAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkDataObjectAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkDataObjectAlgorithm::GetOutput(int port)
{
  return this->GetOutputDataObject(port);
}

//----------------------------------------------------------------------------
void svtkDataObjectAlgorithm::SetOutput(svtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkDataObjectAlgorithm::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkDataObjectAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkDataObjectAlgorithm::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  // Create data object output
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkDataObjectAlgorithm::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int svtkDataObjectAlgorithm::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int svtkDataObjectAlgorithm::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
void svtkDataObjectAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkDataObjectAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void svtkDataObjectAlgorithm::AddInputData(svtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkDataObjectAlgorithm::AddInputData(int index, svtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}
