/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExplicitStructuredGridAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExplicitStructuredGridAlgorithm.h"

#include "svtkCommand.h"
#include "svtkExplicitStructuredGrid.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkExplicitStructuredGridAlgorithm);

//----------------------------------------------------------------------------
svtkExplicitStructuredGridAlgorithm::svtkExplicitStructuredGridAlgorithm()
{
  // by default assume filters have one input and one output
  // subclasses that deviate should modify this setting
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkExplicitStructuredGrid* svtkExplicitStructuredGridAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkExplicitStructuredGrid* svtkExplicitStructuredGridAlgorithm::GetOutput(int port)
{
  return svtkExplicitStructuredGrid::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void svtkExplicitStructuredGridAlgorithm::SetOutput(svtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkExplicitStructuredGridAlgorithm::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkExplicitStructuredGridAlgorithm::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
svtkExplicitStructuredGrid* svtkExplicitStructuredGridAlgorithm::GetExplicitStructuredGridInput(
  int port)
{
  return svtkExplicitStructuredGrid::SafeDownCast(this->GetInput(port));
}

//----------------------------------------------------------------------------
void svtkExplicitStructuredGridAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkExplicitStructuredGridAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void svtkExplicitStructuredGridAlgorithm::AddInputData(svtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkExplicitStructuredGridAlgorithm::AddInputData(int index, svtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkExplicitStructuredGridAlgorithm::ProcessRequest(
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
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    svtkInformation* outputInfo = outputVector->GetInformationObject(0);
    svtkExplicitStructuredGrid* grid = svtkExplicitStructuredGrid::New();
    outputInfo->Set(svtkDataObject::DATA_OBJECT(), grid);
    grid->Delete();
    return 1;
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridAlgorithm::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  // do nothing let subclasses handle it
  return 1;
}

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridAlgorithm::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  int numInputPorts = this->GetNumberOfInputPorts();
  for (int i = 0; i < numInputPorts; i++)
  {
    int numInputConnections = this->GetNumberOfInputConnections(i);
    for (int j = 0; j < numInputConnections; j++)
    {
      svtkInformation* inputInfo = inputVector[i]->GetInformationObject(j);
      inputInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 0);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridAlgorithm::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector))
{
  return 0;
}

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridAlgorithm::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkExplicitStructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridAlgorithm::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkExplicitStructuredGrid");
  return 1;
}
