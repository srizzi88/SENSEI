/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedPolyDataIds.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractSelectedPolyDataIds.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"

svtkStandardNewMacro(svtkExtractSelectedPolyDataIds);

//----------------------------------------------------------------------------
svtkExtractSelectedPolyDataIds::svtkExtractSelectedPolyDataIds()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkExtractSelectedPolyDataIds::~svtkExtractSelectedPolyDataIds() = default;

//----------------------------------------------------------------------------
int svtkExtractSelectedPolyDataIds::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* selInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkSelection* sel = svtkSelection::SafeDownCast(selInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!sel)
  {
    svtkErrorMacro(<< "No selection specified");
    return 1;
  }

  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();

  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();

  svtkDebugMacro(<< "Extracting poly data geometry");

  svtkSelectionNode* node = nullptr;
  if (sel->GetNumberOfNodes() == 1)
  {
    node = sel->GetNode(0);
  }
  if (!node)
  {
    return 1;
  }
  if (!node->GetProperties()->Has(svtkSelectionNode::CONTENT_TYPE()) ||
    node->GetProperties()->Get(svtkSelectionNode::CONTENT_TYPE()) != svtkSelectionNode::INDICES ||
    !node->GetProperties()->Has(svtkSelectionNode::FIELD_TYPE()) ||
    node->GetProperties()->Get(svtkSelectionNode::FIELD_TYPE()) != svtkSelectionNode::CELL)
  {
    return 1;
  }

  svtkIdTypeArray* idArray = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());

  if (!idArray)
  {
    return 1;
  }

  svtkIdType numCells = idArray->GetNumberOfComponents() * idArray->GetNumberOfTuples();

  if (numCells == 0)
  {
    return 1;
  }

  output->AllocateEstimate(numCells, 1);
  output->SetPoints(input->GetPoints());
  outputPD->PassData(pd);
  outputCD->CopyAllocate(cd);

  // Now loop over all cells to see whether they are in the selection.
  // Copy if they are.

  svtkIdList* ids = svtkIdList::New();

  svtkIdType numInputCells = input->GetNumberOfCells();
  for (svtkIdType i = 0; i < numCells; i++)
  {
    svtkIdType cellId = idArray->GetValue(i);
    if (cellId >= numInputCells)
    {
      continue;
    }
    input->GetCellPoints(cellId, ids);
    svtkIdType newId = output->InsertNextCell(input->GetCellType(cellId), ids);
    outputCD->CopyData(cd, cellId, newId);
  }
  ids->Delete();
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedPolyDataIds::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int svtkExtractSelectedPolyDataIds::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  }
  else
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
  }
  return 1;
}
