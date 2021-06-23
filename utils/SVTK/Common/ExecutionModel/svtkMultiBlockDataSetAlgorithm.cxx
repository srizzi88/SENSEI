/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkMultiBlockDataSetAlgorithm.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMultiBlockDataSetAlgorithm.h"

#include "svtkCommand.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkMultiBlockDataSetAlgorithm);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
svtkMultiBlockDataSetAlgorithm::svtkMultiBlockDataSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkMultiBlockDataSet* svtkMultiBlockDataSetAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkMultiBlockDataSet* svtkMultiBlockDataSetAlgorithm::GetOutput(int port)
{
  svtkDataObject* output =
    svtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->GetCompositeOutputData(port);
  return svtkMultiBlockDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
void svtkMultiBlockDataSetAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkMultiBlockDataSetAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkMultiBlockDataSetAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkMultiBlockDataSetAlgorithm::ProcessRequest(
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
int svtkMultiBlockDataSetAlgorithm::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkMultiBlockDataSetAlgorithm::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkMultiBlockDataSetAlgorithm::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void svtkMultiBlockDataSetAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
