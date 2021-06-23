/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataSetAlgorithm.h"

#include "svtkCommand.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredPoints.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkDataSetAlgorithm);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
svtkDataSetAlgorithm::svtkDataSetAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkDataSet* svtkDataSetAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkDataSet* svtkDataSetAlgorithm::GetOutput(int port)
{
  return svtkDataSet::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
// Get the output as svtkImageData
svtkImageData* svtkDataSetAlgorithm::GetImageDataOutput()
{
  return svtkImageData::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkPolyData.
svtkPolyData* svtkDataSetAlgorithm::GetPolyDataOutput()
{
  return svtkPolyData::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkStructuredPoints.
svtkStructuredPoints* svtkDataSetAlgorithm::GetStructuredPointsOutput()
{
  return svtkStructuredPoints::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkStructuredGrid.
svtkStructuredGrid* svtkDataSetAlgorithm::GetStructuredGridOutput()
{
  return svtkStructuredGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkUnstructuredGrid.
svtkUnstructuredGrid* svtkDataSetAlgorithm::GetUnstructuredGridOutput()
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkRectilinearGrid.
svtkRectilinearGrid* svtkDataSetAlgorithm::GetRectilinearGridOutput()
{
  return svtkRectilinearGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
void svtkDataSetAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkDataSetAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void svtkDataSetAlgorithm::SetInputData(svtkDataSet* input)
{
  this->SetInputData(0, static_cast<svtkDataObject*>(input));
}

//----------------------------------------------------------------------------
void svtkDataSetAlgorithm::SetInputData(int index, svtkDataSet* input)
{
  this->SetInputData(index, static_cast<svtkDataObject*>(input));
}

//----------------------------------------------------------------------------
void svtkDataSetAlgorithm::AddInputData(svtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkDataSetAlgorithm::AddInputData(int index, svtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void svtkDataSetAlgorithm::AddInputData(svtkDataSet* input)
{
  this->AddInputData(0, static_cast<svtkDataObject*>(input));
}

//----------------------------------------------------------------------------
void svtkDataSetAlgorithm::AddInputData(int index, svtkDataSet* input)
{
  this->AddInputData(index, static_cast<svtkDataObject*>(input));
}

//----------------------------------------------------------------------------
svtkDataObject* svtkDataSetAlgorithm::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkDataSetAlgorithm::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkDataSetAlgorithm::ProcessRequest(
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
    return this->RequestInformation(request, inputVector, outputVector);
  }

  // set update extent
  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkDataSetAlgorithm::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      svtkInformation* info = outputVector->GetInformationObject(i);
      svtkDataSet* output = svtkDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));

      if (!output || !output->IsA(input->GetClassName()))
      {
        svtkDataSet* newOutput = input->NewInstance();
        info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkDataSetAlgorithm::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkDataSetAlgorithm::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkDataSetAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
