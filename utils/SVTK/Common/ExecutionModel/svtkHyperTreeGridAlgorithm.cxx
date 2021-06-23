/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperTreeGridAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHyperTreeGridAlgorithm.h"

#include "svtkBitArray.h"
#include "svtkCommand.h"
#include "svtkHyperTreeGrid.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
svtkHyperTreeGridAlgorithm::svtkHyperTreeGridAlgorithm()
{
  // By default, only one input and one output ports
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  // Keep references to input and output data
  this->InData = nullptr; // todo: should be a safer pointer type
  this->OutData = nullptr;

  this->AppropriateOutput = false;
}

//----------------------------------------------------------------------------
svtkHyperTreeGridAlgorithm::~svtkHyperTreeGridAlgorithm()
{
  this->InData = nullptr;
  this->OutData = nullptr;
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->InData)
  {
    os << indent << "InData:\n";
    this->InData->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "InData: ( none )\n";
  }

  os << indent << "OutData: ";
  if (this->OutData)
  {
    this->OutData->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

//----------------------------------------------------------------------------
svtkDataObject* svtkHyperTreeGridAlgorithm::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkHyperTreeGridAlgorithm::GetOutput(int port)
{
  return this->GetOutputDataObject(port);
}

//----------------------------------------------------------------------------
svtkHyperTreeGrid* svtkHyperTreeGridAlgorithm::GetHyperTreeGridOutput()
{
  return this->GetHyperTreeGridOutput(0);
}

//----------------------------------------------------------------------------
svtkHyperTreeGrid* svtkHyperTreeGridAlgorithm::GetHyperTreeGridOutput(int port)
{
  return svtkHyperTreeGrid::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
svtkPolyData* svtkHyperTreeGridAlgorithm::GetPolyDataOutput()
{
  return this->GetPolyDataOutput(0);
}

//----------------------------------------------------------------------------
svtkPolyData* svtkHyperTreeGridAlgorithm::GetPolyDataOutput(int port)
{
  return svtkPolyData::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkHyperTreeGridAlgorithm::GetUnstructuredGridOutput()
{
  return this->GetUnstructuredGridOutput(0);
}

//----------------------------------------------------------------------------
svtkUnstructuredGrid* svtkHyperTreeGridAlgorithm::GetUnstructuredGridOutput(int port)
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetOutputDataObject(port));
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridAlgorithm::SetOutput(svtkDataObject* d)
{
  this->GetExecutive()->SetOutputData(0, d);
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridAlgorithm::RequestDataObject(
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
  }
  return 1;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkHyperTreeGridAlgorithm::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->AppropriateOutput)
  {
    // create the output
    if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
      return this->RequestDataObject(request, inputVector, outputVector);
    }
  }

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
int svtkHyperTreeGridAlgorithm::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridAlgorithm::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridAlgorithm::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector*)
{
  // Do nothing and let subclasses handle it if needed
  return 1;
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridAlgorithm::RequestUpdateExtent(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector*)
{
  int numInputPorts = this->GetNumberOfInputPorts();
  for (int i = 0; i < numInputPorts; ++i)
  {
    int numInputConnections = this->GetNumberOfInputConnections(i);
    for (int j = 0; j < numInputConnections; ++j)
    {
      svtkInformation* inputInfo = inputVector[i]->GetInformationObject(j);
      inputInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkHyperTreeGridAlgorithm::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Update progress
  this->UpdateProgress(0.);

  // Retrieve input and output
  svtkHyperTreeGrid* input = svtkHyperTreeGrid::GetData(inputVector[0], 0);
  if (!input)
  {
    svtkErrorMacro("No input available. Cannot proceed with hyper tree grid algorithm.");
    return 0;
  }
  svtkDataObject* outputDO = svtkDataObject::GetData(outputVector, 0);
  if (!outputDO)
  {
    svtkErrorMacro("No output available. Cannot proceed with hyper tree grid algorithm.");
    return 0;
  }

  this->OutData = nullptr; // JB Pourquoi mettre au niveau de Algorithm le OutData ?

  // Process all trees in input grid and generate input data object
  // only if extents are correct
  if ((input->GetExtent()[0] <= input->GetExtent()[1] ||
        input->GetExtent()[2] <= input->GetExtent()[3] ||
        input->GetExtent()[4] <= input->GetExtent()[5]) &&
    !this->ProcessTrees(input, outputDO))
  {
    return 0;
  }

  // Squeeze output data if present
  if (this->OutData)
  {
    this->OutData->Squeeze();
  }

  // Update progress and return
  this->UpdateProgress(1.);
  return 1;
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridAlgorithm::SetInputData(svtkDataObject* input)
{
  this->SetInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridAlgorithm::SetInputData(int index, svtkDataObject* input)
{
  this->SetInputDataInternal(index, input);
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridAlgorithm::AddInputData(svtkDataObject* input)
{
  this->AddInputData(0, input);
}

//----------------------------------------------------------------------------
void svtkHyperTreeGridAlgorithm::AddInputData(int index, svtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}
