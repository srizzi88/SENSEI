/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractSelectedBlock.h"

#include "svtkArrayDispatch.h"
#include "svtkDataArrayRange.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkUnsignedIntArray.h"

#include <unordered_set>
svtkStandardNewMacro(svtkExtractSelectedBlock);
//----------------------------------------------------------------------------
svtkExtractSelectedBlock::svtkExtractSelectedBlock() = default;

//----------------------------------------------------------------------------
svtkExtractSelectedBlock::~svtkExtractSelectedBlock() = default;

//----------------------------------------------------------------------------
int svtkExtractSelectedBlock::FillInputPortInformation(int port, svtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);

  // now add our info
  if (port == 0)
  {
    // Can work with composite datasets.
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  }

  return 1;
}

//----------------------------------------------------------------------------
// Needed because parent class sets output type to input type
// and we sometimes want to change it to make an UnstructuredGrid regardless of
// input type
int svtkExtractSelectedBlock::RequestDataObject(
  svtkInformation* req, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  svtkCompositeDataSet* input = svtkCompositeDataSet::GetData(inInfo);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (input)
  {
    svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outInfo);
    if (!output)
    {
      output = svtkMultiBlockDataSet::New();
      outInfo->Set(svtkDataObject::DATA_OBJECT(), output);
      output->Delete();
    }
    return 1;
  }

  return this->Superclass::RequestDataObject(req, inputVector, outputVector);
}

namespace
{
/**
 * Copies subtree and removes ids for subtree from `ids`.
 */
void svtkCopySubTree(std::unordered_set<unsigned int>& ids, svtkCompositeDataIterator* loc,
  svtkCompositeDataSet* output, svtkCompositeDataSet* input)
{
  svtkDataObject* inputNode = input->GetDataSet(loc);
  if (svtkCompositeDataSet* cinput = svtkCompositeDataSet::SafeDownCast(inputNode))
  {
    svtkCompositeDataSet* coutput = svtkCompositeDataSet::SafeDownCast(output->GetDataSet(loc));
    assert(coutput != nullptr);

    // shallow copy..this pass the non-leaf nodes over.
    coutput->ShallowCopy(cinput);

    // now, we need to remove all composite ids for the subtree from the set to
    // extract to avoid attempting to copy them multiple times (although it
    // should not be harmful at all).

    svtkCompositeDataIterator* iter = cinput->NewIterator();
    if (svtkDataObjectTreeIterator* treeIter = svtkDataObjectTreeIterator::SafeDownCast(iter))
    {
      treeIter->VisitOnlyLeavesOff();
    }
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      ids.erase(loc->GetCurrentFlatIndex() + iter->GetCurrentFlatIndex());
    }
    iter->Delete();
  }
  else
  {
    output->SetDataSet(loc, inputNode);
  }
  ids.erase(loc->GetCurrentFlatIndex());
}
}

namespace
{
struct SelectionToIds
{
  template <typename ArrayT>
  void operator()(ArrayT* array, std::unordered_set<unsigned int>& blocks) const
  {
    for (auto value : svtk::DataArrayValueRange(array))
    {
      blocks.insert(static_cast<unsigned int>(value));
    }
  }
};
} // namespace

//----------------------------------------------------------------------------
int svtkExtractSelectedBlock::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkCompositeDataSet* cd = svtkCompositeDataSet::GetData(inInfo);
  if (!cd)
  {
    svtkDataObject* outputDO = svtkDataObject::GetData(outInfo);
    outputDO->ShallowCopy(svtkDataObject::GetData(inInfo));
    return 1;
  }

  if (!selInfo)
  {
    // When not given a selection, quietly select nothing.
    return 1;
  }

  svtkSelection* input = svtkSelection::GetData(selInfo);
  svtkSelectionNode* node = input->GetNode(0);
  if (input->GetNumberOfNodes() != 1 || node->GetContentType() != svtkSelectionNode::BLOCKS)
  {
    svtkErrorMacro("This filter expects a single-node selection of type BLOCKS.");
    return 0;
  }

  bool inverse = (node->GetProperties()->Has(svtkSelectionNode::INVERSE()) &&
    node->GetProperties()->Get(svtkSelectionNode::INVERSE()) == 1);

  svtkDataArray* selectionList = svtkArrayDownCast<svtkDataArray>(node->GetSelectionList());
  std::unordered_set<unsigned int> blocks;
  if (selectionList)
  {
    using Dispatcher = svtkArrayDispatch::DispatchByValueType<svtkArrayDispatch::Integrals>;
    if (!Dispatcher::Execute(selectionList, SelectionToIds{}, blocks))
    { // fallback for unsupported array types
      // and non-integral value types:
      SelectionToIds{}(selectionList, blocks);
    }
  }

  svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outInfo);

  // short-circuit if root index is present.
  const bool has_root = (blocks.find(0) != blocks.end());
  if (has_root && !inverse)
  {
    // pass everything.
    output->ShallowCopy(cd);
    return 1;
  }

  if (has_root && inverse)
  {
    // pass nothing.
    output->CopyStructure(cd);
    return 1;
  }

  // pass selected ids (or invert)
  output->CopyStructure(cd);

  svtkCompositeDataIterator* citer = cd->NewIterator();
  if (svtkDataObjectTreeIterator* diter = svtkDataObjectTreeIterator::SafeDownCast(citer))
  {
    diter->VisitOnlyLeavesOff();
  }

  for (citer->InitTraversal(); !citer->IsDoneWithTraversal(); citer->GoToNextItem())
  {
    auto fiter = blocks.find(citer->GetCurrentFlatIndex());
    if ((inverse && fiter == blocks.end()) || (!inverse && fiter != blocks.end()))
    {
      svtkCopySubTree(blocks, citer, output, cd);
    }
  }
  citer->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedBlock::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
