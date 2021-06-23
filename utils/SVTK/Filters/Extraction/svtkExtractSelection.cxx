/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractSelection.h"

#include "svtkAssume.h"
#include "svtkBlockSelector.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkExtractCells.h"
#include "svtkFrustumSelector.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLocationSelector.h"
#include "svtkLogger.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSMPTools.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSelector.h"
#include "svtkSignedCharArray.h"
#include "svtkTable.h"
#include "svtkUniformGridAMRDataIterator.h"
#include "svtkUnstructuredGrid.h"
#include "svtkValueSelector.h"

#include <cassert>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <vector>

svtkStandardNewMacro(svtkExtractSelection);
//----------------------------------------------------------------------------
svtkExtractSelection::svtkExtractSelection()
{
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkExtractSelection::~svtkExtractSelection() {}

//----------------------------------------------------------------------------
int svtkExtractSelection::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  }
  else
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractSelection::RequestDataObject(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
  {
    return 0;
  }

  svtkDataObject* inputDO = svtkDataObject::GetData(inputVector[0], 0);
  svtkDataObject* outputDO = svtkDataObject::GetData(outputVector, 0);
  assert(inputDO != nullptr);

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->PreserveTopology)
  {
    // when PreserveTopology is ON, we're preserve input data type.
    if (outputDO == nullptr || !outputDO->IsA(inputDO->GetClassName()))
    {
      outputDO = inputDO->NewInstance();
      outInfo->Set(svtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (svtkCompositeDataSet::SafeDownCast(inputDO))
  {
    // For any composite dataset, we're create a svtkMultiBlockDataSet as output;
    if (svtkMultiBlockDataSet::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = svtkMultiBlockDataSet::New();
      outInfo->Set(svtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (svtkTable::SafeDownCast(inputDO))
  {
    // svtkTable input stays as svtkTable.
    if (svtkTable::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = svtkTable::New();
      outInfo->Set(svtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (svtkDataSet::SafeDownCast(inputDO))
  {
    // svtkDataSet becomes a svtkUnstructuredGrid.
    if (svtkUnstructuredGrid::SafeDownCast(outputDO) == nullptr)
    {
      outputDO = svtkUnstructuredGrid::New();
      outInfo->Set(svtkDataObject::DATA_OBJECT(), outputDO);
      outputDO->Delete();
    }
    return 1;
  }

  if (!outputDO || !outputDO->IsTypeOf(inputDO->GetClassName()))
  {
    outputDO = inputDO->NewInstance();
    outInfo->Set(svtkDataObject::DATA_OBJECT(), outputDO);
    outputDO->Delete();
    return 1;
  }

  svtkErrorMacro("Not sure what type of output to create!");
  return 0;
}

//----------------------------------------------------------------------------
svtkDataObject::AttributeTypes svtkExtractSelection::GetAttributeTypeOfSelection(
  svtkSelection* sel, bool& sane)
{
  sane = true;
  int fieldType = -1;
  for (unsigned int n = 0; n < sel->GetNumberOfNodes(); n++)
  {
    svtkSelectionNode* node = sel->GetNode(n);

    int nodeFieldType = node->GetFieldType();

    if (nodeFieldType == svtkSelectionNode::POINT &&
      node->GetProperties()->Has(svtkSelectionNode::CONTAINING_CELLS()) &&
      node->GetProperties()->Get(svtkSelectionNode::CONTAINING_CELLS()))
    {
      // we're really selecting cells, not points.
      nodeFieldType = svtkSelectionNode::CELL;
    }

    if (n != 0 && fieldType != nodeFieldType)
    {
      sane = false;
      svtkErrorMacro("Selection contains mismatched attribute types!");
      return svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES;
    }

    fieldType = nodeFieldType;
  }

  return fieldType == -1 ? svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES
                         : static_cast<svtkDataObject::AttributeTypes>(
                             svtkSelectionNode::ConvertSelectionFieldToAttributeType(fieldType));
}

namespace
{
void InvertSelection(svtkSignedCharArray* array)
{
  const svtkIdType n = array->GetNumberOfTuples();
  svtkSMPTools::For(0, n, [&array](svtkIdType start, svtkIdType end) {
    for (svtkIdType i = start; i < end; ++i)
    {
      array->SetValue(i, static_cast<signed char>(array->GetValue(i) * -1 + 1));
    }
  });
}
}

//----------------------------------------------------------------------------
int svtkExtractSelection::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDataObject* input = svtkDataObject::GetData(inputVector[0], 0);
  svtkSelection* selection = svtkSelection::GetData(inputVector[1], 0);
  svtkDataObject* output = svtkDataObject::GetData(outputVector, 0);

  // If no input, error
  if (!input)
  {
    svtkErrorMacro(<< "No input specified");
    return 0;
  }

  // If no selection, quietly select nothing
  if (!selection)
  {
    return 1;
  }

  // check for select FieldType consistency right here and return failure
  // if they are not consistent.
  bool sane;
  const auto assoc = this->GetAttributeTypeOfSelection(selection, sane);
  if (!sane || assoc == svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
  {
    svtkErrorMacro("Selection has selection nodes with inconsistent field types.");
    return 0;
  }

  // Create operators for each of svtkSelectionNode instances and initialize them.
  std::map<std::string, svtkSmartPointer<svtkSelector> > selectors;
  for (unsigned int cc = 0, max = selection->GetNumberOfNodes(); cc < max; ++cc)
  {
    auto node = selection->GetNode(cc);
    auto name = selection->GetNodeNameAtIndex(cc);

    if (auto anOperator = this->NewSelectionOperator(
          static_cast<svtkSelectionNode::SelectionContent>(node->GetContentType())))
    {
      anOperator->SetInsidednessArrayName(name.c_str());
      anOperator->Initialize(node);
      selectors[name] = anOperator;
    }
    else
    {
      svtkWarningMacro("Unhandled selection node with content type : " << node->GetContentType());
    }
  }

  auto evaluate = [&selectors, assoc, &selection](svtkDataObject* dobj) {
    auto fieldData = dobj->GetAttributes(assoc);
    if (!fieldData)
    {
      return;
    }

    // Iterate over operators and set up a map from selection node name to insidedness
    // array.
    std::map<std::string, svtkSignedCharArray*> arrayMap;
    for (auto nodeIter = selectors.begin(); nodeIter != selectors.end(); ++nodeIter)
    {
      auto name = nodeIter->first;
      auto insidednessArray = svtkSignedCharArray::SafeDownCast(fieldData->GetArray(name.c_str()));
      auto node = selection->GetNode(name.c_str());
      if (insidednessArray != nullptr && node->GetProperties()->Has(svtkSelectionNode::INVERSE()) &&
        node->GetProperties()->Get(svtkSelectionNode::INVERSE()))
      {
        ::InvertSelection(insidednessArray);
      }
      arrayMap[name] = insidednessArray;
    }

    // Evaluate the map of insidedness arrays
    auto blockInsidedness = selection->Evaluate(arrayMap);
    blockInsidedness->SetName("__svtkInsidedness__");
    fieldData->AddArray(blockInsidedness);
  };

  auto extract = [&assoc, this](
                   svtkDataObject* inpDO, svtkDataObject* opDO) -> svtkSmartPointer<svtkDataObject> {
    auto fd = opDO->GetAttributes(assoc);
    auto array =
      fd ? svtkSignedCharArray::SafeDownCast(fd->GetArray("__svtkInsidedness__")) : nullptr;
    auto resultDO = array ? this->ExtractElements(inpDO, assoc, array) : nullptr;
    return (resultDO && resultDO->GetNumberOfElements(assoc) > 0) ? resultDO : nullptr;
  };

  if (auto inputCD = svtkCompositeDataSet::SafeDownCast(input))
  {
    auto outputCD = svtkCompositeDataSet::SafeDownCast(output);
    assert(outputCD != nullptr);
    outputCD->CopyStructure(inputCD);

    svtkSmartPointer<svtkCompositeDataIterator> inIter;
    inIter.TakeReference(inputCD->NewIterator());

    // Initialize the output composite dataset to have blocks with the same type
    // as the input.
    for (inIter->InitTraversal(); !inIter->IsDoneWithTraversal(); inIter->GoToNextItem())
    {
      auto blockInput = inIter->GetCurrentDataObject();
      if (blockInput)
      {
        auto clone = blockInput->NewInstance();
        clone->ShallowCopy(blockInput);
        outputCD->SetDataSet(inIter, clone);
        clone->FastDelete();
      }
    }

    // Evaluate the operators.
    svtkLogStartScope(TRACE, "execute selectors");
    for (auto nodeIter = selectors.begin(); nodeIter != selectors.end(); ++nodeIter)
    {
      auto selector = nodeIter->second;
      selector->Execute(inputCD, outputCD);
    }
    svtkLogEndScope("execute selectors");

    svtkLogStartScope(TRACE, "evaluate expression");
    // Now iterate again over the composite dataset and evaluate the expression to
    // combine all the insidedness arrays.
    svtkSmartPointer<svtkCompositeDataIterator> outIter;
    outIter.TakeReference(outputCD->NewIterator());
    for (outIter->GoToFirstItem(); !outIter->IsDoneWithTraversal(); outIter->GoToNextItem())
    {
      auto outputBlock = outIter->GetCurrentDataObject();
      assert(outputBlock != nullptr);
      // Evaluate the expression.
      evaluate(outputBlock);
    }
    svtkLogEndScope("evaluate expression");

    svtkLogStartScope(TRACE, "extract output");
    for (outIter->GoToFirstItem(); !outIter->IsDoneWithTraversal(); outIter->GoToNextItem())
    {
      outputCD->SetDataSet(
        outIter, extract(inputCD->GetDataSet(outIter), outIter->GetCurrentDataObject()));
    }
    svtkLogEndScope("extract output");
  }
  else
  {
    assert(output != nullptr);

    svtkSmartPointer<svtkDataObject> clone;
    clone.TakeReference(input->NewInstance());
    clone->ShallowCopy(input);

    // Evaluate the operators.
    svtkLogStartScope(TRACE, "execute selectors");
    for (auto nodeIter = selectors.begin(); nodeIter != selectors.end(); ++nodeIter)
    {
      auto selector = nodeIter->second;
      selector->Execute(input, clone);
    }
    svtkLogEndScope("execute selectors");

    svtkLogStartScope(TRACE, "evaluate expression");
    evaluate(clone);
    svtkLogEndScope("evaluate expression");

    svtkLogStartScope(TRACE, "extract output");
    if (auto result = extract(input, clone))
    {
      output->ShallowCopy(result);
    }
    svtkLogEndScope("extract output");
  }

  return 1;
}

//----------------------------------------------------------------------------
svtkSmartPointer<svtkSelector> svtkExtractSelection::NewSelectionOperator(
  svtkSelectionNode::SelectionContent contentType)
{
  switch (contentType)
  {
    case svtkSelectionNode::GLOBALIDS:
    case svtkSelectionNode::PEDIGREEIDS:
    case svtkSelectionNode::VALUES:
    case svtkSelectionNode::INDICES:
    case svtkSelectionNode::THRESHOLDS:
      return svtkSmartPointer<svtkValueSelector>::New();

    case svtkSelectionNode::FRUSTUM:
      return svtkSmartPointer<svtkFrustumSelector>::New();

    case svtkSelectionNode::LOCATIONS:
      return svtkSmartPointer<svtkLocationSelector>::New();

    case svtkSelectionNode::BLOCKS:
      return svtkSmartPointer<svtkBlockSelector>::New();

    case svtkSelectionNode::USER:
    case svtkSelectionNode::SELECTIONS:
    case svtkSelectionNode::QUERY:
    default:
      return nullptr;
  }
}

//----------------------------------------------------------------------------
svtkSmartPointer<svtkDataObject> svtkExtractSelection::ExtractElements(
  svtkDataObject* block, svtkDataObject::AttributeTypes type, svtkSignedCharArray* insidednessArray)
{
  if (this->PreserveTopology)
  {
    auto output = block->NewInstance();
    output->ShallowCopy(block);
    insidednessArray->SetName("svtkInsidedness");
    output->GetAttributesAsFieldData(type)->AddArray(insidednessArray);
    return svtkSmartPointer<svtkDataObject>::Take(output);
  }
  if (type == svtkDataObject::POINT)
  {
    svtkDataSet* input = svtkDataSet::SafeDownCast(block);
    if (!input)
    {
      return nullptr;
    }
    auto output = svtkUnstructuredGrid::New();
    this->ExtractSelectedPoints(input, output, insidednessArray);
    return svtkSmartPointer<svtkDataSet>::Take(output);
  }
  else if (type == svtkDataObject::CELL)
  {
    svtkDataSet* input = svtkDataSet::SafeDownCast(block);
    if (!input)
    {
      return nullptr;
    }
    auto output = svtkUnstructuredGrid::New();
    this->ExtractSelectedCells(input, output, insidednessArray);
    return svtkSmartPointer<svtkDataSet>::Take(output);
  }
  else if (type == svtkDataObject::ROW)
  {
    svtkTable* input = svtkTable::SafeDownCast(block);
    if (!input)
    {
      return nullptr;
    }
    svtkTable* output = svtkTable::New();
    this->ExtractSelectedRows(input, output, insidednessArray);
    return svtkSmartPointer<svtkTable>::Take(output);
  }

  svtkDataObject* output = block->NewInstance();
  return svtkSmartPointer<svtkDataObject>::Take(output);
}

//----------------------------------------------------------------------------
void svtkExtractSelection::ExtractSelectedCells(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkSignedCharArray* cellInside)
{
  svtkLogScopeF(TRACE, "ExtractSelectedCells");
  const svtkIdType numPts = input->GetNumberOfPoints();
  const svtkIdType numCells = input->GetNumberOfCells();

  if (!cellInside || cellInside->GetNumberOfTuples() <= 0)
  {
    // Assume nothing was selected and return.
    return;
  }

  assert(cellInside->GetNumberOfTuples() == numCells);

  const auto range = cellInside->GetValueRange(0);
  if (range[0] == 0 && range[1] == 0)
  {
    // all elements are being masked out, nothing to do.
    return;
  }

  // The "input" is a shallow copy of the input to this filter and hence we can
  // modify it. We add original cell ids and point ids arrays.
  svtkNew<svtkIdTypeArray> originalPointIds;
  originalPointIds->SetNumberOfComponents(1);
  originalPointIds->SetName("svtkOriginalPointIds");
  originalPointIds->SetNumberOfTuples(numPts);
  std::iota(originalPointIds->GetPointer(0), originalPointIds->GetPointer(0) + numPts, 0);
  input->GetPointData()->AddArray(originalPointIds);

  svtkNew<svtkIdTypeArray> originalCellIds;
  originalCellIds->SetNumberOfComponents(1);
  originalCellIds->SetName("svtkOriginalCellIds");
  originalCellIds->SetNumberOfTuples(numCells);
  std::iota(originalCellIds->GetPointer(0), originalCellIds->GetPointer(0) + numCells, 0);
  input->GetCellData()->AddArray(originalCellIds);

  svtkNew<svtkExtractCells> extractor;
  if (range[0] == 1 && range[1] == 1)
  {
    // all elements are selected, pass all data.
    // we still use the extractor since it does the data conversion, if needed
    extractor->SetExtractAllCells(true);
  }
  else
  {
    // convert insideness array to cell ids to extract.
    std::vector<svtkIdType> ids;
    ids.reserve(numCells);
    for (svtkIdType cc = 0; cc < numCells; ++cc)
    {
      if (cellInside->GetValue(cc) != 0)
      {
        ids.push_back(cc);
      }
    }
    extractor->SetAssumeSortedAndUniqueIds(true);
    extractor->SetCellIds(&ids.front(), static_cast<svtkIdType>(ids.size()));
  }

  extractor->SetInputDataObject(input);
  extractor->Update();
  output->ShallowCopy(extractor->GetOutput());
}

//----------------------------------------------------------------------------
void svtkExtractSelection::ExtractSelectedPoints(
  svtkDataSet* input, svtkUnstructuredGrid* output, svtkSignedCharArray* pointInside)
{
  if (!pointInside || pointInside->GetNumberOfTuples() <= 0)
  {
    // Assume nothing was selected and return.
    return;
  }

  svtkIdType numPts = input->GetNumberOfPoints();

  svtkPointData* pd = input->GetPointData();
  svtkPointData* outputPD = output->GetPointData();

  // To copy points in a type agnostic way later
  auto pointSet = svtkPointSet::SafeDownCast(input);

  svtkNew<svtkPoints> newPts;
  if (pointSet)
  {
    newPts->SetDataType(pointSet->GetPoints()->GetDataType());
  }
  newPts->Allocate(numPts / 4, numPts);

  svtkNew<svtkIdList> newCellPts;
  newCellPts->Allocate(SVTK_CELL_SIZE);

  outputPD->SetCopyGlobalIds(1);
  outputPD->CopyFieldOff("svtkOriginalPointIds");
  outputPD->CopyAllocate(pd);

  double x[3];

  svtkNew<svtkIdTypeArray> originalPointIds;
  originalPointIds->SetNumberOfComponents(1);
  originalPointIds->SetName("svtkOriginalPointIds");
  outputPD->AddArray(originalPointIds);

  for (svtkIdType ptId = 0; ptId < numPts; ++ptId)
  {
    signed char isInside;
    assert(ptId < pointInside->GetNumberOfValues());
    pointInside->GetTypedTuple(ptId, &isInside);
    if (isInside)
    {
      svtkIdType newPointId = -1;
      if (pointSet)
      {
        newPointId = newPts->GetNumberOfPoints();
        newPts->InsertPoints(newPointId, 1, ptId, pointSet->GetPoints());
      }
      else
      {
        input->GetPoint(ptId, x);
        newPointId = newPts->InsertNextPoint(x);
      }
      assert(newPointId >= 0);
      outputPD->CopyData(pd, ptId, newPointId);
      originalPointIds->InsertNextValue(ptId);
    }
  }

  // produce a new svtk_vertex cell for each accepted point
  for (svtkIdType ptId = 0; ptId < newPts->GetNumberOfPoints(); ++ptId)
  {
    newCellPts->Reset();
    newCellPts->InsertId(0, ptId);
    output->InsertNextCell(SVTK_VERTEX, newCellPts);
  }
  output->SetPoints(newPts);
}

//----------------------------------------------------------------------------
void svtkExtractSelection::ExtractSelectedRows(
  svtkTable* input, svtkTable* output, svtkSignedCharArray* rowsInside)
{
  const svtkIdType numRows = input->GetNumberOfRows();
  svtkNew<svtkIdTypeArray> originalRowIds;
  originalRowIds->SetName("svtkOriginalRowIds");

  output->GetRowData()->CopyFieldOff("svtkOriginalRowIds");
  output->GetRowData()->CopyStructure(input->GetRowData());

  for (svtkIdType rowId = 0; rowId < numRows; ++rowId)
  {
    signed char isInside;
    rowsInside->GetTypedTuple(rowId, &isInside);
    if (isInside)
    {
      output->InsertNextRow(input->GetRow(rowId));
      originalRowIds->InsertNextValue(rowId);
    }
  }
  output->AddColumn(originalRowIds);
}

//----------------------------------------------------------------------------
void svtkExtractSelection::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PreserveTopology: " << this->PreserveTopology << endl;
}
