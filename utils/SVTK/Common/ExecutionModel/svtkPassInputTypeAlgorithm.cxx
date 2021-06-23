/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPassInputTypeAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPassInputTypeAlgorithm.h"

#include "svtkCommand.h"
#include "svtkDataObject.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMolecule.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredPoints.h"
#include "svtkTable.h"
#include "svtkUnstructuredGrid.h"

svtkStandardNewMacro(svtkPassInputTypeAlgorithm);

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
svtkPassInputTypeAlgorithm::svtkPassInputTypeAlgorithm()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkPassInputTypeAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkPassInputTypeAlgorithm::GetOutput(int port)
{
  return svtkDataObject::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
// Get the output as svtkImageData
svtkImageData* svtkPassInputTypeAlgorithm::GetImageDataOutput()
{
  return svtkImageData::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkPolyData.
svtkPolyData* svtkPassInputTypeAlgorithm::GetPolyDataOutput()
{
  return svtkPolyData::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkStructuredPoints.
svtkStructuredPoints* svtkPassInputTypeAlgorithm::GetStructuredPointsOutput()
{
  return svtkStructuredPoints::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkStructuredGrid.
svtkStructuredGrid* svtkPassInputTypeAlgorithm::GetStructuredGridOutput()
{
  return svtkStructuredGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkUnstructuredGrid.
svtkUnstructuredGrid* svtkPassInputTypeAlgorithm::GetUnstructuredGridOutput()
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkRectilinearGrid.
svtkRectilinearGrid* svtkPassInputTypeAlgorithm::GetRectilinearGridOutput()
{
  return svtkRectilinearGrid::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkGraph.
svtkGraph* svtkPassInputTypeAlgorithm::GetGraphOutput()
{
  return svtkGraph::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkMolecule.
svtkMolecule* svtkPassInputTypeAlgorithm::GetMoleculeOutput()
{
  return svtkMolecule::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
// Get the output as svtkTable.
svtkTable* svtkPassInputTypeAlgorithm::GetTableOutput()
{
  return svtkTable::SafeDownCast(this->GetOutput());
}

//----------------------------------------------------------------------------
void svtkPassInputTypeAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkPassInputTypeAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void svtkPassInputTypeAlgorithm::AddInputData(svtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkPassInputTypeAlgorithm::AddInputData(int index, svtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkPassInputTypeAlgorithm::GetInput()
{
  return this->GetInput(0);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkPassInputTypeAlgorithm::GetInput(int port)
{
  return this->GetExecutive()->GetInputData(port, 0);
}

//----------------------------------------------------------------------------
svtkTypeBool svtkPassInputTypeAlgorithm::ProcessRequest(
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

  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_TIME()))
  {
    this->RequestUpdateTime(request, inputVector, outputVector);
  }

  if (request->Has(svtkStreamingDemandDrivenPipeline::REQUEST_TIME_DEPENDENT_INFORMATION()))
  {
    this->RequestUpdateTimeDependentInformation(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkPassInputTypeAlgorithm::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->GetNumberOfInputPorts() == 0 || this->GetNumberOfOutputPorts() == 0)
  {
    return 1;
  }

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());

  if (input)
  {
    // for each output
    for (int i = 0; i < this->GetNumberOfOutputPorts(); ++i)
    {
      svtkInformation* info = outputVector->GetInformationObject(i);
      svtkDataObject* output = info->Get(svtkDataObject::DATA_OBJECT());

      if (!output || !output->IsA(input->GetClassName()))
      {
        svtkDataObject* newOutput = input->NewInstance();
        info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
        newOutput->Delete();
      }
    }
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkPassInputTypeAlgorithm::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int svtkPassInputTypeAlgorithm::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void svtkPassInputTypeAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
