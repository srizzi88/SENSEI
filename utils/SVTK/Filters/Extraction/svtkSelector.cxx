/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkSelector.h"

#include "svtkCompositeDataSet.h"
#include "svtkDataObjectTree.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkExpandMarkedElements.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkSMPTools.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkUniformGrid.h"
#include "svtkUniformGridAMR.h"
#include "svtkUniformGridAMRDataIterator.h"

//----------------------------------------------------------------------------
svtkSelector::svtkSelector()
  : InsidednessArrayName()
{
}

//----------------------------------------------------------------------------
svtkSelector::~svtkSelector() {}

//----------------------------------------------------------------------------
void svtkSelector::Initialize(svtkSelectionNode* node)
{
  this->Node = node;
}

//--------------------------------------------------------------------------
void svtkSelector::ProcessBlock(
  svtkDataObject* inputBlock, svtkDataObject* outputBlock, bool forceFalse)
{
  assert(svtkCompositeDataSet::SafeDownCast(inputBlock) == nullptr &&
    svtkCompositeDataSet::SafeDownCast(outputBlock) == nullptr);

  int association =
    svtkSelectionNode::ConvertSelectionFieldToAttributeType(this->Node->GetFieldType());

  const svtkIdType numElements = inputBlock->GetNumberOfElements(association);
  auto insidednessArray = this->CreateInsidednessArray(numElements);
  if (forceFalse || !this->ComputeSelectedElements(inputBlock, insidednessArray))
  {
    insidednessArray->FillValue(0);
  }

  // If selecting cells containing points, we need to map the selected points
  // to selected cells.
  auto selectionProperties = this->Node->GetProperties();
  if (association == svtkDataObject::POINT &&
    selectionProperties->Has(svtkSelectionNode::CONTAINING_CELLS()) &&
    selectionProperties->Get(svtkSelectionNode::CONTAINING_CELLS()) == 1)
  {
    // convert point insidednessArray to cell-based insidednessArray
    insidednessArray = this->ComputeCellsContainingSelectedPoints(inputBlock, insidednessArray);
    association = svtkDataObject::CELL;
  }

  auto dsa = outputBlock->GetAttributes(association);
  if (dsa && insidednessArray)
  {
    dsa->AddArray(insidednessArray);
  }
}

//--------------------------------------------------------------------------
void svtkSelector::Execute(svtkDataObject* input, svtkDataObject* output)
{
  if (svtkCompositeDataSet::SafeDownCast(input))
  {
    assert(svtkCompositeDataSet::SafeDownCast(output) != nullptr);
    auto inputDOT = svtkDataObjectTree::SafeDownCast(input);
    auto outputDOT = svtkDataObjectTree::SafeDownCast(output);
    if (inputDOT && outputDOT)
    {
      this->ProcessDataObjectTree(inputDOT, outputDOT, this->GetBlockSelection(0), 0);
    }
    else
    {
      auto inputAMR = svtkUniformGridAMR::SafeDownCast(input);
      auto outputCD = svtkCompositeDataSet::SafeDownCast(output);
      if (inputAMR && outputCD)
      {
        this->ProcessAMR(inputAMR, outputCD);
      }
    }
  }
  else
  {
    this->ProcessBlock(input, output, false);
  }

  // handle expanding to connected elements.
  this->ExpandToConnectedElements(output);
}

//--------------------------------------------------------------------------
void svtkSelector::ExpandToConnectedElements(svtkDataObject* output)
{
  // Expand layers, if requested.
  auto selectionProperties = this->Node->GetProperties();
  if (selectionProperties->Has(svtkSelectionNode::CONNECTED_LAYERS()))
  {
    int association =
      svtkSelectionNode::ConvertSelectionFieldToAttributeType(this->Node->GetFieldType());
    // If selecting cells containing points, we need to map the selected points
    // to selected cells.
    if (association == svtkDataObject::POINT &&
      selectionProperties->Has(svtkSelectionNode::CONTAINING_CELLS()) &&
      selectionProperties->Get(svtkSelectionNode::CONTAINING_CELLS()) == 1)
    {
      association = svtkDataObject::CELL;
    }

    const int layers = selectionProperties->Get(svtkSelectionNode::CONNECTED_LAYERS());
    if (layers >= 1 && (association == svtkDataObject::POINT || association == svtkDataObject::CELL))
    {
      svtkNew<svtkExpandMarkedElements> expander;
      expander->SetInputArrayToProcess(0, 0, 0, association, this->InsidednessArrayName.c_str());
      expander->SetNumberOfLayers(layers);
      expander->SetInputDataObject(output);
      expander->Update();
      output->ShallowCopy(expander->GetOutputDataObject(0));
    }
  }
}

//----------------------------------------------------------------------------
void svtkSelector::ProcessDataObjectTree(svtkDataObjectTree* input, svtkDataObjectTree* output,
  svtkSelector::SelectionMode mode, unsigned int compositeIndex)
{
  auto iter = input->NewTreeIterator();
  iter->TraverseSubTreeOff();
  iter->VisitOnlyLeavesOff();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto inputDO = iter->GetCurrentDataObject();
    auto outputDO = output->GetDataSet(iter);
    if (inputDO && outputDO)
    {
      const auto currentIndex = compositeIndex + iter->GetCurrentFlatIndex();

      auto blockMode = this->GetBlockSelection(currentIndex);
      blockMode = (blockMode == INHERIT) ? mode : blockMode;

      auto inputDT = svtkDataObjectTree::SafeDownCast(inputDO);
      auto outputDT = svtkDataObjectTree::SafeDownCast(outputDO);
      if (inputDT && outputDT)
      {
        this->ProcessDataObjectTree(inputDT, outputDT, blockMode, currentIndex);
      }
      else
      {

        this->ProcessBlock(inputDO, outputDO, blockMode == EXCLUDE);
      }
    }
  }
  iter->Delete();
}

//----------------------------------------------------------------------------
void svtkSelector::ProcessAMR(svtkUniformGridAMR* input, svtkCompositeDataSet* output)
{
  auto iter = svtkUniformGridAMRDataIterator::SafeDownCast(input->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto modeDSUsingCompositeIndex = this->GetBlockSelection(iter->GetCurrentFlatIndex());
    auto modeDSUsingAMRLevel =
      this->GetAMRBlockSelection(iter->GetCurrentLevel(), iter->GetCurrentIndex());
    auto realMode =
      modeDSUsingAMRLevel == INHERIT ? modeDSUsingCompositeIndex : modeDSUsingAMRLevel;
    realMode = (realMode == INHERIT) ? EXCLUDE : realMode;

    auto inputDS = iter->GetCurrentDataObject();
    auto outputDS = output->GetDataSet(iter);
    if (inputDS && outputDS)
    {
      this->ProcessBlock(inputDS, outputDS, realMode == EXCLUDE);
    }
  }

  iter->Delete();
}

//----------------------------------------------------------------------------
svtkSelector::SelectionMode svtkSelector::GetAMRBlockSelection(unsigned int level, unsigned int index)
{
  auto properties = this->Node->GetProperties();
  auto levelKey = svtkSelectionNode::HIERARCHICAL_LEVEL();
  auto indexKey = svtkSelectionNode::HIERARCHICAL_INDEX();
  const auto hasLevel = properties->Has(levelKey);
  const auto hasIndex = properties->Has(indexKey);
  if (!hasLevel && !hasIndex)
  {
    return INHERIT;
  }
  else if (hasLevel && !hasIndex)
  {
    return static_cast<unsigned int>(properties->Get(levelKey)) == level ? INCLUDE : EXCLUDE;
  }
  else if (hasIndex && !hasLevel)
  {
    return static_cast<unsigned int>(properties->Get(indexKey)) == index ? INCLUDE : EXCLUDE;
  }
  else
  {
    assert(hasIndex && hasLevel);
    return static_cast<unsigned int>(properties->Get(levelKey)) == level &&
        static_cast<unsigned int>(properties->Get(indexKey)) == index
      ? INCLUDE
      : EXCLUDE;
  }
}

//----------------------------------------------------------------------------
svtkSelector::SelectionMode svtkSelector::GetBlockSelection(unsigned int compositeIndex)
{
  auto properties = this->Node->GetProperties();
  auto key = svtkSelectionNode::COMPOSITE_INDEX();
  if (properties->Has(key))
  {
    if (static_cast<unsigned int>(properties->Get(key)) == compositeIndex)
    {
      return INCLUDE;
    }
    else
    {
      // this needs some explanation:
      // if `COMPOSITE_INDEX` is present, then the root node is to be treated as
      // excluded unless explicitly selected. This ensures that
      // we only "INCLUDE" the chosen subtree(s).
      // For all other nodes, we will simply return INHERIT, that way the state
      // from the parent is inherited unless overridden.
      return compositeIndex == 0 ? EXCLUDE : INHERIT;
    }
  }
  else
  {
    return INHERIT;
  }
}

//----------------------------------------------------------------------------
// Creates a new insidedness array with the given number of elements.
svtkSmartPointer<svtkSignedCharArray> svtkSelector::CreateInsidednessArray(svtkIdType numElems)
{
  auto darray = svtkSmartPointer<svtkSignedCharArray>::New();
  darray->SetName(this->InsidednessArrayName.c_str());
  darray->SetNumberOfComponents(1);
  darray->SetNumberOfTuples(numElems);
  return darray;
}

//----------------------------------------------------------------------------
svtkSmartPointer<svtkSignedCharArray> svtkSelector::ComputeCellsContainingSelectedPoints(
  svtkDataObject* data, svtkSignedCharArray* selectedPoints)
{
  svtkDataSet* dataset = svtkDataSet::SafeDownCast(data);
  if (!dataset)
  {
    return nullptr;
  }

  const svtkIdType numCells = dataset->GetNumberOfCells();
  auto selectedCells = this->CreateInsidednessArray(numCells);

  if (numCells > 0)
  {
    // call once to make the GetCellPoints call thread safe.
    svtkNew<svtkIdList> cellPts;
    dataset->GetCellPoints(0, cellPts);
  }

  // run through cells and accept those with any point inside
  svtkSMPTools::For(0, numCells, [&](svtkIdType first, svtkIdType last) {
    svtkNew<svtkIdList> cellPts;
    for (svtkIdType cellId = first; cellId < last; ++cellId)
    {
      dataset->GetCellPoints(cellId, cellPts);
      const svtkIdType numCellPts = cellPts->GetNumberOfIds();
      signed char selectedPointFound = 0;
      for (svtkIdType i = 0; i < numCellPts; ++i)
      {
        svtkIdType ptId = cellPts->GetId(i);
        if (selectedPoints->GetValue(ptId) != 0)
        {
          selectedPointFound = 1;
          break;
        }
      }
      selectedCells->SetValue(cellId, selectedPointFound);
    }
  });

  return selectedCells;
}

//----------------------------------------------------------------------------
void svtkSelector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InsidednessArrayName: " << this->InsidednessArrayName << endl;
}
