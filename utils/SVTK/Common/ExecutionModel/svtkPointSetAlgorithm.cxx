/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointSetAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointSetAlgorithm.h"

#include "svtkCommand.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkPointSetAlgorithm);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
svtkPointSetAlgorithm::svtkPointSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkPointSet* svtkPointSetAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkPointSet* svtkPointSetAlgorithm::GetOutput(int port)
{
  return svtkPointSet::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
// Get the output as svtkPolyData.
svtkPolyData* svtkPointSetAlgorithm::GetPolyDataOutput()
{
  return svtkPolyData::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkStructuredGrid.
svtkStructuredGrid* svtkPointSetAlgorithm::GetStructuredGridOutput()
{
  return svtkStructuredGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkUnstructuredGrid.
svtkUnstructuredGrid* svtkPointSetAlgorithm::GetUnstructuredGridOutput()
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
void svtkPointSetAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkPointSetAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void svtkPointSetAlgorithm::SetInputData(svtkPointSet* input)
{
  this->SetInputData(0, static_cast<svtkDataObject*>(input));
}

//----------------------------------------------------------------------------
void svtkPointSetAlgorithm::SetInputData(int index, svtkPointSet* input)
{
  this->SetInputData(index, static_cast<svtkDataObject*>(input));
}

//----------------------------------------------------------------------------
void svtkPointSetAlgorithm::AddInputData(svtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkPointSetAlgorithm::AddInputData(int index, svtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void svtkPointSetAlgorithm::AddInputData(svtkPointSet* input)
{
  this->AddInputData(0, static_cast<svtkDataObject*>(input));
}

//----------------------------------------------------------------------------
void svtkPointSetAlgorithm::AddInputData(int index, svtkPointSet* input)
{
  this->AddInputData(index, static_cast<svtkDataObject*>(input));
}

//----------------------------------------------------------------------------
svtkDataObject* svtkPointSetAlgorithm::GetInput()
{
  return this->GetExecutive()->GetInputData(0, 0);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkPointSetAlgorithm::ProcessRequest(
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

  // execute information
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->ExecuteInformation(request, inputVector, outputVector);
  }

  // set update extent
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->ComputeInputUpdateExtent(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkPointSetAlgorithm::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      svtkInformation* info = outputVector->GetInformationObject(i);
      svtkPointSet* output = svtkPointSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));

      if (!output || !output->IsA(input->GetClassName()))
      {
        output = input->NewInstance();
        info->Set(svtkDataObject::DATA_OBJECT(), output);
        output->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkPointSetAlgorithm::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkPointSetAlgorithm::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkPointSetAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
