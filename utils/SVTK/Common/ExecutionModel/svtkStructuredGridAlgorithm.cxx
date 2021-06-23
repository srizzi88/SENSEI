/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredGridAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStructuredGridAlgorithm.h"

#include "svtkCommand.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"

svtkStandardNewMacro(svtkStructuredGridAlgorithm);

//----------------------------------------------------------------------------
svtkStructuredGridAlgorithm::svtkStructuredGridAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkStructuredGridAlgorithm::~svtkStructuredGridAlgorithm() = default;

//----------------------------------------------------------------------------
void svtkStructuredGridAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkStructuredGridAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkStructuredGridAlgorithm::GetOutput(int port)
{
  return svtkStructuredGrid::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void svtkStructuredGridAlgorithm::SetOutput(svtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkStructuredGridAlgorithm::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkStructuredGridAlgorithm::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
svtkStructuredGrid* svtkStructuredGridAlgorithm::GetStructuredGridInput(int port)
{
  return svtkStructuredGrid::SafeDownCast(this->GetInput(port));
}

//----------------------------------------------------------------------------
svtkTypeBool svtkStructuredGridAlgorithm::ProcessRequest(
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

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkStructuredGridAlgorithm::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkStructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkStructuredGridAlgorithm::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkStructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkStructuredGridAlgorithm::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int svtkStructuredGridAlgorithm::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  return 0;
}

//----------------------------------------------------------------------------
void svtkStructuredGridAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkStructuredGridAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void svtkStructuredGridAlgorithm::AddInputData(svtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkStructuredGridAlgorithm::AddInputData(int index, svtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}
