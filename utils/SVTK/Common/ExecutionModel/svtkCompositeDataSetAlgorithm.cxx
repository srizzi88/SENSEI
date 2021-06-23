/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkCompositeDataSetAlgorithm.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCompositeDataSetAlgorithm.h"

#include "svtkCommand.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkCompositeDataSetAlgorithm);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
svtkCompositeDataSetAlgorithm::svtkCompositeDataSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkCompositeDataSet* svtkCompositeDataSetAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkCompositeDataSet* svtkCompositeDataSetAlgorithm::GetOutput(int port)
{
  svtkDataObject* output =
    svtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->GetCompositeOutputData(port);
  return svtkCompositeDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
void svtkCompositeDataSetAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkCompositeDataSetAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkCompositeDataSetAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkCompositeDataSetAlgorithm::ProcessRequest(
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
    return retVal;
  }

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  // set update extent
  if (request->Has(svtkCompositeDataPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkCompositeDataSetAlgorithm::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkCompositeDataSetAlgorithm::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkCompositeDataSetAlgorithm::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void svtkCompositeDataSetAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
