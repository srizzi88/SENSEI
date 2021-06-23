/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkDataSetGhostGenerator.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkDataSetGhostGenerator.h"
#include "svtkDataObject.h"
#include "svtkIndent.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"

#include <cassert>

svtkDataSetGhostGenerator::svtkDataSetGhostGenerator()
{
  this->NumberOfGhostLayers = 0;
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//------------------------------------------------------------------------------
svtkDataSetGhostGenerator::~svtkDataSetGhostGenerator() = default;

//------------------------------------------------------------------------------
void svtkDataSetGhostGenerator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "NumberOfGhostLayers: " << this->NumberOfGhostLayers << std::endl;
}

//------------------------------------------------------------------------------
int svtkDataSetGhostGenerator::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  assert("pre: information object is nullptr!" && (info != nullptr));
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int svtkDataSetGhostGenerator::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  assert("pre: information object is nullptr!" && (info != nullptr));

  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int svtkDataSetGhostGenerator::RequestData(svtkInformation* svtkNotUsed(rqst),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  // STEP 0: Get input object
  svtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: Null input object!" && (input != nullptr));
  svtkMultiBlockDataSet* inputMultiBlock =
    svtkMultiBlockDataSet::SafeDownCast(input->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: Input multi-block object is nullptr" && (inputMultiBlock != nullptr));

  // STEP 1: Get output object
  svtkInformation* output = outputVector->GetInformationObject(0);
  assert("pre: Null output object!" && (output != nullptr));
  svtkMultiBlockDataSet* outputMultiBlock =
    svtkMultiBlockDataSet::SafeDownCast(output->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: Outpu multi-block object is nullptr" && (outputMultiBlock != nullptr));

  // STEP 2: Generate ghost layers
  if (this->NumberOfGhostLayers == 0)
  {
    // Shallow copy the input object
    outputMultiBlock->ShallowCopy(inputMultiBlock);
  }
  else
  {
    // Create requested ghost layers
    this->GenerateGhostLayers(inputMultiBlock, outputMultiBlock);
  }
  return 1;
}
