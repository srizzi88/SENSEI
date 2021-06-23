/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearGridAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRectilinearGridAlgorithm.h"

#include "svtkCommand.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkRectilinearGridAlgorithm);

//----------------------------------------------------------------------------
svtkRectilinearGridAlgorithm::svtkRectilinearGridAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkRectilinearGridAlgorithm::~svtkRectilinearGridAlgorithm() = default;

//----------------------------------------------------------------------------
void svtkRectilinearGridAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkRectilinearGridAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkRectilinearGridAlgorithm::GetOutput(int port)
{
  return svtkRectilinearGrid::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void svtkRectilinearGridAlgorithm::SetOutput(svtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkRectilinearGridAlgorithm::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkRectilinearGridAlgorithm::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
svtkRectilinearGrid* svtkRectilinearGridAlgorithm::GetRectilinearGridInput(int port)
{
  return svtkRectilinearGrid::SafeDownCast(this->GetInput(port));
}

//----------------------------------------------------------------------------
svtkTypeBool svtkRectilinearGridAlgorithm::ProcessRequest(
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
int svtkRectilinearGridAlgorithm::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkRectilinearGridAlgorithm::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkRectilinearGridAlgorithm::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
// This is the superclasses style of Execute method.  Convert it into
// an imaging style Execute method.
int svtkRectilinearGridAlgorithm::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  return 0;
}

//----------------------------------------------------------------------------
void svtkRectilinearGridAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkRectilinearGridAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void svtkRectilinearGridAlgorithm::AddInputData(svtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkRectilinearGridAlgorithm::AddInputData(int index, svtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}
