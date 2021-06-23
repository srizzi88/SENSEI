/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractBlock.h"

#include "svtkDataObjectTreeIterator.h"
#include "svtkDataSet.h"
#include "svtkFieldData.h"
#include "svtkInformation.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkObjectFactory.h"

#include <set>

class svtkExtractBlock::svtkSet : public std::set<unsigned int>
{
};

svtkStandardNewMacro(svtkExtractBlock);
svtkInformationKeyMacro(svtkExtractBlock, DONT_PRUNE, Integer);
//----------------------------------------------------------------------------
svtkExtractBlock::svtkExtractBlock()
{
  this->Indices = new svtkExtractBlock::svtkSet();
  this->ActiveIndices = new svtkExtractBlock::svtkSet();
  this->PruneOutput = 1;
  this->MaintainStructure = 0;
}

//----------------------------------------------------------------------------
svtkExtractBlock::~svtkExtractBlock()
{
  delete this->Indices;
  delete this->ActiveIndices;
}

//----------------------------------------------------------------------------
void svtkExtractBlock::AddIndex(unsigned int index)
{
  this->Indices->insert(index);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkExtractBlock::RemoveIndex(unsigned int index)
{
  this->Indices->erase(index);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkExtractBlock::RemoveAllIndices()
{
  this->Indices->clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkExtractBlock::CopySubTree(
  svtkDataObjectTreeIterator* loc, svtkMultiBlockDataSet* output, svtkMultiBlockDataSet* input)
{
  svtkDataObject* inputNode = input->GetDataSet(loc);
  if (!inputNode->IsA("svtkCompositeDataSet"))
  {
    svtkDataObject* clone = inputNode->NewInstance();
    clone->ShallowCopy(inputNode);
    output->SetDataSet(loc, clone);
    clone->Delete();
  }
  else
  {
    svtkCompositeDataSet* cinput = svtkCompositeDataSet::SafeDownCast(inputNode);
    svtkCompositeDataSet* coutput = svtkCompositeDataSet::SafeDownCast(output->GetDataSet(loc));
    svtkCompositeDataIterator* iter = cinput->NewIterator();
    svtkDataObjectTreeIterator* treeIter = svtkDataObjectTreeIterator::SafeDownCast(iter);
    if (treeIter)
    {
      treeIter->VisitOnlyLeavesOff();
    }
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataObject* curNode = iter->GetCurrentDataObject();
      svtkDataObject* clone = curNode->NewInstance();
      clone->ShallowCopy(curNode);
      coutput->SetDataSet(iter, clone);
      clone->Delete();

      this->ActiveIndices->erase(loc->GetCurrentFlatIndex() + iter->GetCurrentFlatIndex());
    }
    iter->Delete();
  }
}

//----------------------------------------------------------------------------
int svtkExtractBlock::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkMultiBlockDataSet* input = svtkMultiBlockDataSet::GetData(inputVector[0], 0);
  svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outputVector, 0);

  svtkDebugMacro(<< "Extracting blocks");

  if (this->Indices->find(0) != this->Indices->end())
  {
    // trivial case.
    output->ShallowCopy(input);
    return 1;
  }

  output->CopyStructure(input);

  (*this->ActiveIndices) = (*this->Indices);

  // Copy selected blocks over to the output.
  svtkDataObjectTreeIterator* iter = input->NewTreeIterator();
  iter->VisitOnlyLeavesOff();

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal() && !this->ActiveIndices->empty();
       iter->GoToNextItem())
  {
    if (this->ActiveIndices->find(iter->GetCurrentFlatIndex()) != this->ActiveIndices->end())
    {
      this->ActiveIndices->erase(iter->GetCurrentFlatIndex());

      // This removed the visited indices from this->ActiveIndices.
      this->CopySubTree(iter, output, input);
    }
  }
  iter->Delete();
  this->ActiveIndices->clear();

  if (!this->PruneOutput)
  {
    return 1;
  }

  // Now prune the output tree.

  // Since in case multiple processes are involved, this process may have some
  // data-set pointers nullptr. Hence, pruning cannot simply trim nullptr ptrs, since
  // in that case we may end up with different structures on different
  // processes, which is a big NO-NO. Hence, we first flag nodes based on
  // whether they are being pruned or not.

  iter = output->NewTreeIterator();
  iter->VisitOnlyLeavesOff();
  iter->SkipEmptyNodesOff();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (this->Indices->find(iter->GetCurrentFlatIndex()) != this->Indices->end())
    {
      iter->GetCurrentMetaData()->Set(DONT_PRUNE(), 1);
    }
    else if (iter->HasCurrentMetaData() && iter->GetCurrentMetaData()->Has(DONT_PRUNE()))
    {
      iter->GetCurrentMetaData()->Remove(DONT_PRUNE());
    }
  }
  iter->Delete();

  // Do the actual pruning. Only those branches are pruned which don't have
  // DONT_PRUNE flag set.
  this->Prune(output);
  return 1;
}

//----------------------------------------------------------------------------
bool svtkExtractBlock::Prune(svtkDataObject* branch)
{
  if (branch->IsA("svtkMultiBlockDataSet"))
  {
    return this->Prune(svtkMultiBlockDataSet::SafeDownCast(branch));
  }
  else if (branch->IsA("svtkMultiPieceDataSet"))
  {
    return this->Prune(svtkMultiPieceDataSet::SafeDownCast(branch));
  }

  return true;
}

//----------------------------------------------------------------------------
bool svtkExtractBlock::Prune(svtkMultiPieceDataSet* mpiece)
{
  // * Remove any children on mpiece that don't have DONT_PRUNE set.
  svtkMultiPieceDataSet* clone = svtkMultiPieceDataSet::New();

  // Copy global field data, otherwise it will be lost
  clone->GetFieldData()->ShallowCopy(mpiece->GetFieldData());

  unsigned int index = 0;
  unsigned int numChildren = mpiece->GetNumberOfPieces();
  for (unsigned int cc = 0; cc < numChildren; cc++)
  {
    if (mpiece->HasMetaData(cc) && mpiece->GetMetaData(cc)->Has(DONT_PRUNE()))
    {
      clone->SetPiece(index, mpiece->GetPiece(cc));
      clone->GetMetaData(index)->Copy(mpiece->GetMetaData(cc));
      index++;
    }
  }
  mpiece->ShallowCopy(clone);
  clone->Delete();

  // tell caller to prune mpiece away if num of pieces is 0.
  return (mpiece->GetNumberOfPieces() == 0);
}

//----------------------------------------------------------------------------
bool svtkExtractBlock::Prune(svtkMultiBlockDataSet* mblock)
{
  svtkMultiBlockDataSet* clone = svtkMultiBlockDataSet::New();

  // Copy global field data, otherwise it will be lost
  clone->GetFieldData()->ShallowCopy(mblock->GetFieldData());

  unsigned int index = 0;
  unsigned int numChildren = mblock->GetNumberOfBlocks();
  for (unsigned int cc = 0; cc < numChildren; cc++)
  {
    svtkDataObject* block = mblock->GetBlock(cc);
    if (mblock->HasMetaData(cc) && mblock->GetMetaData(cc)->Has(DONT_PRUNE()))
    {
      clone->SetBlock(index, block);
      clone->GetMetaData(index)->Copy(mblock->GetMetaData(cc));
      index++;
    }
    else if (block)
    {
      bool prune = this->Prune(block);
      if (!prune)
      {
        svtkMultiBlockDataSet* prunedBlock = svtkMultiBlockDataSet::SafeDownCast(block);
        if (this->MaintainStructure == 0 && prunedBlock && prunedBlock->GetNumberOfBlocks() == 1)
        {
          // shrink redundant branches.
          clone->SetBlock(index, prunedBlock->GetBlock(0));
          if (prunedBlock->HasMetaData(static_cast<unsigned int>(0)))
          {
            clone->GetMetaData(index)->Copy(prunedBlock->GetMetaData(static_cast<unsigned int>(0)));
          }
        }
        else
        {
          clone->SetBlock(index, block);
          if (mblock->HasMetaData(cc))
          {
            clone->GetMetaData(index)->Copy(mblock->GetMetaData(cc));
          }
        }
        index++;
      }
    }
  }
  mblock->ShallowCopy(clone);
  clone->Delete();
  return (mblock->GetNumberOfBlocks() == 0);
}

//----------------------------------------------------------------------------
void svtkExtractBlock::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PruneOutput: " << this->PruneOutput << endl;
  os << indent << "MaintainStructure: " << this->MaintainStructure << endl;
}
