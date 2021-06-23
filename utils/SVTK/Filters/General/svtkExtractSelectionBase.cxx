/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectionBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractSelectionBase.h"

#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkTable.h"
#include "svtkUnstructuredGrid.h"

//----------------------------------------------------------------------------
svtkExtractSelectionBase::svtkExtractSelectionBase()
{
  this->PreserveTopology = 0;
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkExtractSelectionBase::~svtkExtractSelectionBase() = default;

//----------------------------------------------------------------------------
int svtkExtractSelectionBase::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    // Cannot work with composite datasets.
    info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  }
  else
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
// Needed because parent class sets output type to input type
// and we sometimes want to change it to make an UnstructuredGrid regardless of
// input type
int svtkExtractSelectionBase::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  svtkDataSet* input = svtkDataSet::GetData(inInfo);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (input)
  {
    int passThrough = this->PreserveTopology ? 1 : 0;

    svtkDataSet* output = svtkDataSet::GetData(outInfo);
    if (!output || (passThrough && !output->IsA(input->GetClassName())) ||
      (!passThrough && !output->IsA("svtkUnstructuredGrid")))
    {
      svtkDataSet* newOutput = nullptr;
      if (!passThrough)
      {
        // The mesh will be modified.
        newOutput = svtkUnstructuredGrid::New();
      }
      else
      {
        // The mesh will not be modified.
        newOutput = input->NewInstance();
      }
      outInfo->Set(svtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
    return 1;
  }

  svtkGraph* graphInput = svtkGraph::GetData(inInfo);
  if (graphInput)
  {
    // Accept graph input, but we don't produce the correct extracted
    // graph as output yet.
    return 1;
  }

  svtkTable* tableInput = svtkTable::GetData(inInfo);
  if (tableInput)
  {
    svtkTable* output = svtkTable::GetData(outInfo);
    if (!output)
    {
      output = svtkTable::New();
      outInfo->Set(svtkDataObject::DATA_OBJECT(), output);
      output->Delete();
    }
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
void svtkExtractSelectionBase::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PreserveTopology: " << this->PreserveTopology << endl;
}
