/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockDataGroupFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMultiBlockDataGroupFilter.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkMultiBlockDataGroupFilter);
//-----------------------------------------------------------------------------
svtkMultiBlockDataGroupFilter::svtkMultiBlockDataGroupFilter() = default;

//-----------------------------------------------------------------------------
svtkMultiBlockDataGroupFilter::~svtkMultiBlockDataGroupFilter() = default;

//-----------------------------------------------------------------------------
int svtkMultiBlockDataGroupFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  svtkInformation* info = outputVector->GetInformationObject(0);
  info->Remove(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  return 1;
}

//-----------------------------------------------------------------------------
int svtkMultiBlockDataGroupFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  for (int i = 0; i < numInputs; i++)
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(i);
    if (inInfo->Has(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int svtkMultiBlockDataGroupFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkMultiBlockDataSet* output =
    svtkMultiBlockDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }

  /*
  unsigned int updatePiece = static_cast<unsigned int>(
    info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  unsigned int updateNumPieces =  static_cast<unsigned int>(
    info->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  */

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  output->SetNumberOfBlocks(numInputs);
  for (int idx = 0; idx < numInputs; ++idx)
  {
    /*
    // This can be a svtkMultiPieceDataSet if we ever support it.
    svtkMultiBlockDataSet* block = svtkMultiBlockDataSet::New();
    block->SetNumberOfBlocks(updateNumPieces);
    block->Delete();
    */

    svtkDataObject* input = nullptr;
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    if (inInfo)
    {
      input = inInfo->Get(svtkDataObject::DATA_OBJECT());
    }
    if (input)
    {
      svtkDataObject* dsCopy = input->NewInstance();
      dsCopy->ShallowCopy(input);
      output->SetBlock(idx, dsCopy);
      dsCopy->Delete();
    }
    else
    {
      output->SetBlock(idx, nullptr);
    }
  }

  if (output->GetNumberOfBlocks() == 1 && output->GetBlock(0) &&
    output->GetBlock(0)->IsA("svtkMultiBlockDataSet"))
  {
    svtkMultiBlockDataSet* block = svtkMultiBlockDataSet::SafeDownCast(output->GetBlock(0));
    block->Register(this);
    output->ShallowCopy(block);
    block->UnRegister(this);
  }

  return 1;
}

//-----------------------------------------------------------------------------
void svtkMultiBlockDataGroupFilter::AddInputData(svtkDataObject* input)
{
  this->AddInputData(0, input);
}

//-----------------------------------------------------------------------------
void svtkMultiBlockDataGroupFilter::AddInputData(int index, svtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}

//-----------------------------------------------------------------------------
int svtkMultiBlockDataGroupFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
void svtkMultiBlockDataGroupFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
