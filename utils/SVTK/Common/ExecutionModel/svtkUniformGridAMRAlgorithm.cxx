/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkUniformGridAMRAlgorithm.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkUniformGridAMRAlgorithm.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDemandDrivenPipeline.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUniformGridAMR.h"

svtkStandardNewMacro(svtkUniformGridAMRAlgorithm);

//------------------------------------------------------------------------------
svtkUniformGridAMRAlgorithm::svtkUniformGridAMRAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
svtkUniformGridAMRAlgorithm::~svtkUniformGridAMRAlgorithm() = default;

//------------------------------------------------------------------------------
void svtkUniformGridAMRAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
svtkUniformGridAMR* svtkUniformGridAMRAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//------------------------------------------------------------------------------
svtkUniformGridAMR* svtkUniformGridAMRAlgorithm::GetOutput(int port)
{
  svtkDataObject* output =
    svtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->GetCompositeOutputData(port);
  return (svtkUniformGridAMR::SafeDownCast(output));
}

//------------------------------------------------------------------------------
void svtkUniformGridAMRAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//------------------------------------------------------------------------------
void svtkUniformGridAMRAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//------------------------------------------------------------------------------
svtkTypeBool svtkUniformGridAMRAlgorithm::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // create the output
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  // generate the data
  if (request->Has(svtkCompositeDataPipeline::REQUEST_DATA()))
  {
    int retVal = this->RequestData(request, inputVector, outputVector);
    return (retVal);
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  // set update extent
  if (request->Has(svtkCompositeDataPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return (this->RequestUpdateExtent(request, inputVector, outputVector));
  }

  return (this->Superclass::ProcessRequest(request, inputVector, outputVector));
}

//------------------------------------------------------------------------------
svtkExecutive* svtkUniformGridAMRAlgorithm::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//------------------------------------------------------------------------------
int svtkUniformGridAMRAlgorithm::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkUniformGridAMR");
  return 1;
}

//------------------------------------------------------------------------------
int svtkUniformGridAMRAlgorithm::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkUniformGridAMR");
  return 1;
}

//------------------------------------------------------------------------------
svtkDataObject* svtkUniformGridAMRAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(port, 0);
}
