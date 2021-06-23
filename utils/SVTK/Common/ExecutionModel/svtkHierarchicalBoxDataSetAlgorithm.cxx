/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalBoxDataSetAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHierarchicalBoxDataSetAlgorithm.h"

#include "svtkCompositeDataPipeline.h"
#include "svtkDataSet.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkHierarchicalBoxDataSetAlgorithm);
//----------------------------------------------------------------------------
svtkHierarchicalBoxDataSetAlgorithm::svtkHierarchicalBoxDataSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkHierarchicalBoxDataSetAlgorithm::~svtkHierarchicalBoxDataSetAlgorithm() = default;

//----------------------------------------------------------------------------
svtkHierarchicalBoxDataSet* svtkHierarchicalBoxDataSetAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkHierarchicalBoxDataSet* svtkHierarchicalBoxDataSetAlgorithm::GetOutput(int port)
{
  svtkDataObject* output =
    svtkCompositeDataPipeline::SafeDownCast(this->GetExecutive())->GetCompositeOutputData(port);
  return svtkHierarchicalBoxDataSet::SafeDownCast(output);
}

//----------------------------------------------------------------------------
void svtkHierarchicalBoxDataSetAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkHierarchicalBoxDataSetAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkHierarchicalBoxDataSetAlgorithm::GetInput(int port)
{
  if (this->GetNumberOfInputConnections(port) < 1)
  {
    return nullptr;
  }
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkHierarchicalBoxDataSetAlgorithm::ProcessRequest(
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
int svtkHierarchicalBoxDataSetAlgorithm::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkHierarchicalBoxDataSetAlgorithm::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
svtkExecutive* svtkHierarchicalBoxDataSetAlgorithm::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void svtkHierarchicalBoxDataSetAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
