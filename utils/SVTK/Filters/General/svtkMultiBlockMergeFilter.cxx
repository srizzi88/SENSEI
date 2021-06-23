/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockMergeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMultiBlockMergeFilter.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkMultiBlockMergeFilter);
//-----------------------------------------------------------------------------
svtkMultiBlockMergeFilter::svtkMultiBlockMergeFilter() = default;

//-----------------------------------------------------------------------------
svtkMultiBlockMergeFilter::~svtkMultiBlockMergeFilter() = default;

//-----------------------------------------------------------------------------
int svtkMultiBlockMergeFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkMultiBlockDataSet* output =
    svtkMultiBlockDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    return 0;
  }

  int numInputs = inputVector[0]->GetNumberOfInformationObjects();
  if (numInputs < 0)
  {
    svtkErrorMacro("Too many inputs to algorithm.");
    return 0;
  }
  unsigned int usNInputs = static_cast<unsigned int>(numInputs);

  int first = 1;
  for (int idx = 0; idx < numInputs; ++idx)
  {
    svtkMultiBlockDataSet* input = nullptr;
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(idx);
    if (inInfo)
    {
      input = svtkMultiBlockDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
    }
    if (input)
    {
      if (first)
      {
        // shallow copy first input to output to start off with
        // cerr << "Copy first input" << endl;
        output->ShallowCopy(svtkMultiBlockDataSet::SafeDownCast(input));
        first = 0;
      }
      else
      {
        if (!this->Merge(usNInputs, idx, output, input))
        {
          return 0;
        }
        /*
        //merge in the inputs data sets into the proper places in the output
        for (unsigned int blk = 0; blk < input->GetNumberOfBlocks(); blk++)
          {
          //inputs are allowed to have either 1 or N datasets in each group
          unsigned int dsId = 0;
          if (input->GetNumberOfDataSets(blk) == usNInputs)
            {
            dsId = idx;
            }
          //cerr << "Copying blk " << blk << " index " << idx << endl;
          svtkDataObject* inblk = input->GetDataSet(blk, dsId);
          svtkDataObject* dsCopy = inblk->NewInstance();
          dsCopy->ShallowCopy(inblk);
          //output will always have N datasets in each group
          if (output->GetNumberOfDataSets(blk) != usNInputs)
            {
            output->SetNumberOfDataSets(blk, numInputs);
            }
          output->SetDataSet(blk, idx, dsCopy);
          dsCopy->Delete();
          }
          */
      }
    }
  }

  return !first;
}

//-----------------------------------------------------------------------------
int svtkMultiBlockMergeFilter::IsMultiPiece(svtkMultiBlockDataSet* mb)
{
  unsigned int numBlocks = mb->GetNumberOfBlocks();
  for (unsigned int cc = 0; cc < numBlocks; cc++)
  {
    svtkDataObject* block = mb->GetBlock(cc);
    if (block && !block->IsA("svtkDataSet"))
    {
      return 0;
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int svtkMultiBlockMergeFilter::Merge(unsigned int numPieces, unsigned int pieceNo,
  svtkMultiBlockDataSet* output, svtkMultiBlockDataSet* input)
{
  if (!input && !output)
  {
    return 1;
  }

  if (!input || !output)
  {
    svtkErrorMacro("Case not handled");
    return 0;
  }

  unsigned int numInBlocks = input->GetNumberOfBlocks();
  unsigned int numOutBlocks = output->GetNumberOfBlocks();

  // Current limitation of this filter is that all blocks must either be
  // svtkMultiBlockDataSet or svtkDataSet not a mixture of the two.
  // a svtkMultiBlockDataSet with all child blocks as svtkDataSet is a multipiece
  // dataset. This filter merges pieces together.
  int mpInput = this->IsMultiPiece(input);
  int mpOutput = this->IsMultiPiece(output);

  if (!mpInput && !mpOutput && (numInBlocks == numOutBlocks))
  {
    for (unsigned int cc = 0; cc < numInBlocks; cc++)
    {
      if (!this->Merge(numPieces, pieceNo, svtkMultiBlockDataSet::SafeDownCast(output->GetBlock(cc)),
            svtkMultiBlockDataSet::SafeDownCast(input->GetBlock(cc))))
      {
        return 0;
      }
    }
    return 1;
  }
  else if (mpInput && mpOutput)
  {
    output->SetNumberOfBlocks(numPieces);
    unsigned int inIndex = 0;
    // inputs are allowed to have either 1 or N datasets in each group
    if (numInBlocks == numPieces)
    {
      inIndex = pieceNo;
    }
    else if (numInBlocks != 1)
    {
      svtkErrorMacro("Case not currently handled.");
      return 0;
    }
    output->SetBlock(pieceNo, svtkDataSet::SafeDownCast(input->GetBlock(inIndex)));
    return 1;
  }

  svtkErrorMacro("Case not currently handled.");
  return 0;
}

//-----------------------------------------------------------------------------
void svtkMultiBlockMergeFilter::AddInputData(svtkDataObject* input)
{
  this->AddInputData(0, input);
}

//-----------------------------------------------------------------------------
void svtkMultiBlockMergeFilter::AddInputData(int index, svtkDataObject* input)
{
  this->AddInputDataInternal(index, input);
}

//-----------------------------------------------------------------------------
int svtkMultiBlockMergeFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkMultiBlockDataSet");
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
  info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
void svtkMultiBlockMergeFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
