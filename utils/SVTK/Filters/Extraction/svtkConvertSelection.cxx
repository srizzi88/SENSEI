/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkConvertSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "svtkConvertSelection.h"

#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkExtractSelectedThresholds.h"
#include "svtkExtractSelection.h"
#include "svtkFieldData.h"
#include "svtkGraph.h"
#include "svtkHierarchicalBoxDataIterator.h"
#include "svtkHierarchicalBoxDataSet.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkUnsignedIntArray.h"
#include "svtkVariantArray.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <set>
#include <vector>

#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkCxxSetObjectMacro(svtkConvertSelection, ArrayNames, svtkStringArray);
svtkCxxSetObjectMacro(svtkConvertSelection, SelectionExtractor, svtkExtractSelection);

svtkStandardNewMacro(svtkConvertSelection);
//----------------------------------------------------------------------------
svtkConvertSelection::svtkConvertSelection()
{
  this->SetNumberOfInputPorts(2);
  this->OutputType = svtkSelectionNode::INDICES;
  this->ArrayNames = nullptr;
  this->InputFieldType = -1;
  this->MatchAnyValues = false;
  this->AllowMissingArray = false;
  this->SelectionExtractor = nullptr;
}

//----------------------------------------------------------------------------
svtkConvertSelection::~svtkConvertSelection()
{
  this->SetArrayNames(nullptr);
  this->SetSelectionExtractor(nullptr);
}

//----------------------------------------------------------------------------
void svtkConvertSelection::AddArrayName(const char* name)
{
  if (!this->ArrayNames)
  {
    this->ArrayNames = svtkStringArray::New();
  }
  this->ArrayNames->InsertNextValue(name);
}

//----------------------------------------------------------------------------
void svtkConvertSelection::ClearArrayNames()
{
  if (this->ArrayNames)
  {
    this->ArrayNames->Initialize();
  }
}

//----------------------------------------------------------------------------
void svtkConvertSelection::SetArrayName(const char* name)
{
  if (!this->ArrayNames)
  {
    this->ArrayNames = svtkStringArray::New();
  }
  this->ArrayNames->Initialize();
  this->ArrayNames->InsertNextValue(name);
}

//----------------------------------------------------------------------------
const char* svtkConvertSelection::GetArrayName()
{
  if (this->ArrayNames && this->ArrayNames->GetNumberOfValues() > 0)
  {
    return this->ArrayNames->GetValue(0);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int svtkConvertSelection::SelectTableFromTable(
  svtkTable* selTable, svtkTable* dataTable, svtkIdTypeArray* indices)
{
  std::set<svtkIdType> matching;
  SVTK_CREATE(svtkIdList, list);
  for (svtkIdType row = 0; row < selTable->GetNumberOfRows(); row++)
  {
    matching.clear();
    bool initialized = false;
    for (svtkIdType col = 0; col < selTable->GetNumberOfColumns(); col++)
    {
      svtkAbstractArray* from = selTable->GetColumn(col);
      svtkAbstractArray* to = dataTable->GetColumnByName(from->GetName());
      if (to)
      {
        to->LookupValue(selTable->GetValue(row, col), list);
        if (!initialized)
        {
          matching.insert(list->GetPointer(0), list->GetPointer(0) + list->GetNumberOfIds());
          initialized = true;
        }
        else
        {
          std::set<svtkIdType> intersection;
          std::sort(list->GetPointer(0), list->GetPointer(0) + list->GetNumberOfIds());
          std::set_intersection(matching.begin(), matching.end(), list->GetPointer(0),
            list->GetPointer(0) + list->GetNumberOfIds(),
            std::inserter(intersection, intersection.begin()));
          matching = intersection;
        }
      }
    }
    std::set<svtkIdType>::iterator it, itEnd = matching.end();
    for (it = matching.begin(); it != itEnd; ++it)
    {
      indices->InsertNextValue(*it);
    }
    if (row % 100 == 0)
    {
      double progress = 0.8 * row / selTable->GetNumberOfRows();
      this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkConvertSelection::ConvertToIndexSelection(
  svtkSelectionNode* input, svtkDataSet* data, svtkSelectionNode* output)
{
  svtkSmartPointer<svtkSelection> tempInput = svtkSmartPointer<svtkSelection>::New();
  tempInput->AddNode(input);

  // Use the extraction filter to create an insidedness array.
  svtkExtractSelection* extract = this->SelectionExtractor;
  extract->PreserveTopologyOn();
  extract->SetInputData(0, data);
  extract->SetInputData(1, tempInput);
  extract->Update();
  svtkDataSet* const extracted = svtkDataSet::SafeDownCast(extract->GetOutput());

  output->SetContentType(svtkSelectionNode::INDICES);
  int type = input->GetFieldType();
  output->SetFieldType(type);
  svtkSignedCharArray* insidedness = nullptr;
  if (type == svtkSelectionNode::CELL)
  {
    insidedness = svtkArrayDownCast<svtkSignedCharArray>(
      extracted->GetCellData()->GetAbstractArray("svtkInsidedness"));
  }
  else if (type == svtkSelectionNode::POINT)
  {
    insidedness = svtkArrayDownCast<svtkSignedCharArray>(
      extracted->GetPointData()->GetAbstractArray("svtkInsidedness"));
  }
  else
  {
    svtkErrorMacro("Unknown field type");
    return 0;
  }

  if (!insidedness)
  {
    // Empty selection
    return 0;
  }

  // Convert the insidedness array into an index input.
  svtkSmartPointer<svtkIdTypeArray> indexArray = svtkSmartPointer<svtkIdTypeArray>::New();
  for (svtkIdType i = 0; i < insidedness->GetNumberOfTuples(); i++)
  {
    if (insidedness->GetValue(i) == 1)
    {
      indexArray->InsertNextValue(i);
    }
  }
  output->SetSelectionList(indexArray);
  return 1;
}

//----------------------------------------------------------------------------
int svtkConvertSelection::ConvertToBlockSelection(
  svtkSelection* input, svtkCompositeDataSet* data, svtkSelection* output)
{
  std::set<unsigned int> indices;
  for (unsigned int n = 0; n < input->GetNumberOfNodes(); ++n)
  {
    svtkSmartPointer<svtkSelectionNode> inputNode = input->GetNode(n);
    if (inputNode->GetContentType() == svtkSelectionNode::GLOBALIDS)
    {
      // global id selection does not have COMPOSITE_INDEX() key, so we convert
      // it to an index base selection, so that we can determine the composite
      // indices.
      svtkSmartPointer<svtkSelection> tempSel = svtkSmartPointer<svtkSelection>::New();
      tempSel->AddNode(inputNode);
      svtkSmartPointer<svtkSelection> tempOutput;
      tempOutput.TakeReference(svtkConvertSelection::ToIndexSelection(tempSel, data));
      inputNode = tempOutput->GetNode(0);
    }
    svtkInformation* properties = inputNode->GetProperties();
    if (properties->Has(svtkSelectionNode::CONTENT_TYPE()) &&
      properties->Has(svtkSelectionNode::COMPOSITE_INDEX()))
    {
      indices.insert(
        static_cast<unsigned int>(properties->Get(svtkSelectionNode::COMPOSITE_INDEX())));
    }
    else if (properties->Has(svtkSelectionNode::CONTENT_TYPE()) &&
      properties->Has(svtkSelectionNode::HIERARCHICAL_INDEX()) &&
      properties->Has(svtkSelectionNode::HIERARCHICAL_LEVEL()) &&
      data->IsA("svtkHierarchicalBoxDataSet"))
    {
      // convert hierarchical index to composite index.
      svtkHierarchicalBoxDataSet* hbox = static_cast<svtkHierarchicalBoxDataSet*>(data);
      indices.insert(hbox->GetCompositeIndex(
        static_cast<unsigned int>(properties->Get(svtkSelectionNode::HIERARCHICAL_LEVEL())),
        static_cast<unsigned int>(properties->Get(svtkSelectionNode::HIERARCHICAL_INDEX()))));
    }
  }

  svtkSmartPointer<svtkUnsignedIntArray> selectionList = svtkSmartPointer<svtkUnsignedIntArray>::New();
  selectionList->SetNumberOfTuples(static_cast<svtkIdType>(indices.size()));
  std::set<unsigned int>::iterator siter;
  svtkIdType index = 0;
  for (siter = indices.begin(); siter != indices.end(); ++siter, ++index)
  {
    selectionList->SetValue(index, *siter);
  }
  svtkSmartPointer<svtkSelectionNode> outputNode = svtkSmartPointer<svtkSelectionNode>::New();
  outputNode->SetContentType(svtkSelectionNode::BLOCKS);
  outputNode->SetSelectionList(selectionList);
  output->AddNode(outputNode);
  return 1;
}

//----------------------------------------------------------------------------
int svtkConvertSelection::ConvertCompositeDataSet(
  svtkSelection* input, svtkCompositeDataSet* data, svtkSelection* output)
{
  // If this->OutputType == svtkSelectionNode::BLOCKS we just want to create a new
  // selection with the chosen block indices.
  if (this->OutputType == svtkSelectionNode::BLOCKS)
  {
    return this->ConvertToBlockSelection(input, data, output);
  }

  for (unsigned int n = 0; n < input->GetNumberOfNodes(); ++n)
  {
    svtkSelectionNode* inputNode = input->GetNode(n);

    // *  If input has no composite keys then it implies that it applies to all
    //    nodes in the data. If input has composite keys, output will have
    //    composite keys unless outputContentType == GLOBALIDS.
    //    If input does not have composite keys, then composite
    //    keys are only added for outputContentType == INDICES, FRUSTUM and
    //    PEDIGREEIDS.
    bool has_composite_key =
      inputNode->GetProperties()->Has(svtkSelectionNode::COMPOSITE_INDEX()) != 0;

    unsigned int composite_index = has_composite_key
      ? static_cast<unsigned int>(
          inputNode->GetProperties()->Get(svtkSelectionNode::COMPOSITE_INDEX()))
      : 0;

    bool has_hierarchical_key =
      inputNode->GetProperties()->Has(svtkSelectionNode::HIERARCHICAL_INDEX()) != 0 &&
      inputNode->GetProperties()->Has(svtkSelectionNode::HIERARCHICAL_LEVEL()) != 0;

    unsigned int hierarchical_level = has_hierarchical_key
      ? static_cast<unsigned int>(
          inputNode->GetProperties()->Get(svtkSelectionNode::HIERARCHICAL_LEVEL()))
      : 0;
    unsigned int hierarchical_index = has_hierarchical_key
      ? static_cast<unsigned int>(
          inputNode->GetProperties()->Get(svtkSelectionNode::HIERARCHICAL_INDEX()))
      : 0;

    if ((!has_composite_key && !has_hierarchical_key) &&
      inputNode->GetContentType() == svtkSelectionNode::QUERY &&
      this->OutputType == svtkSelectionNode::INDICES)
    {
      this->ConvertFromQueryNodeCompositeDataSet(inputNode, data, output);
      continue;
    }

    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(data->NewIterator());

    svtkHierarchicalBoxDataIterator* hbIter = svtkHierarchicalBoxDataIterator::SafeDownCast(iter);

    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (has_hierarchical_key && hbIter &&
        (hbIter->GetCurrentLevel() != hierarchical_level ||
          hbIter->GetCurrentIndex() != hierarchical_index))
      {
        continue;
      }

      if (has_composite_key && iter->GetCurrentFlatIndex() != composite_index)
      {
        continue;
      }

      svtkSmartPointer<svtkSelection> outputNodes = svtkSmartPointer<svtkSelection>::New();
      svtkSmartPointer<svtkSelection> tempSel = svtkSmartPointer<svtkSelection>::New();
      tempSel->AddNode(inputNode);
      if (!this->Convert(tempSel, iter->GetCurrentDataObject(), outputNodes))
      {
        return 0;
      }

      for (unsigned int j = 0; j < outputNodes->GetNumberOfNodes(); ++j)
      {
        svtkSelectionNode* outputNode = outputNodes->GetNode(j);
        if ((has_hierarchical_key || has_composite_key ||
              this->OutputType == svtkSelectionNode::INDICES ||
              this->OutputType == svtkSelectionNode::PEDIGREEIDS ||
              this->OutputType == svtkSelectionNode::FRUSTUM) &&
          this->OutputType != svtkSelectionNode::GLOBALIDS)
        {
          outputNode->GetProperties()->Set(
            svtkSelectionNode::COMPOSITE_INDEX(), iter->GetCurrentFlatIndex());

          if (has_hierarchical_key && hbIter)
          {
            outputNode->GetProperties()->Set(
              svtkSelectionNode::HIERARCHICAL_LEVEL(), hierarchical_level);
            outputNode->GetProperties()->Set(
              svtkSelectionNode::HIERARCHICAL_INDEX(), hierarchical_index);
          }
        }
        output->Union(outputNode);
      } // for each output node
    }   // for each block
  }     // for each input selection node

  return 1;
}

//----------------------------------------------------------------------------
int svtkConvertSelection::ConvertFromQueryNodeCompositeDataSet(
  svtkSelectionNode* inputNode, svtkCompositeDataSet* data, svtkSelection* output)
{
  // QUERY selection types with composite data input need special handling.
  // The query can apply to a composite dataset, so we extract the selection
  // on the entire dataset here and convert it to an index selection.
  svtkNew<svtkSelection> tempSelection;
  tempSelection->AddNode(inputNode);
  svtkExtractSelection* extract = this->SelectionExtractor;
  extract->PreserveTopologyOn();
  extract->SetInputData(0, data);
  extract->SetInputData(1, tempSelection);
  extract->Update();

  svtkDataObject* extracted = extract->GetOutput();
  svtkCompositeDataSet* cds = svtkCompositeDataSet::SafeDownCast(extracted);
  if (cds)
  {
    svtkSmartPointer<svtkCompositeDataIterator> iter;
    iter.TakeReference(cds->NewIterator());

    svtkHierarchicalBoxDataIterator* hbIter = svtkHierarchicalBoxDataIterator::SafeDownCast(iter);

    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      svtkDataSet* dataset = svtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (!dataset)
      {
        continue;
      }

      auto inputProperties = inputNode->GetProperties();
      bool hasCompositeKey = inputProperties->Has(svtkSelectionNode::COMPOSITE_INDEX()) != 0;
      bool hasHierarchicalKey = inputProperties->Has(svtkSelectionNode::HIERARCHICAL_INDEX()) != 0 &&
        inputProperties->Has(svtkSelectionNode::HIERARCHICAL_LEVEL()) != 0;

      // Create a selection node for the block
      svtkNew<svtkSelectionNode> outputNode;
      outputNode->SetFieldType(inputNode->GetFieldType());
      outputNode->SetContentType(svtkSelectionNode::INDICES);
      auto outputProperties = outputNode->GetProperties();
      outputProperties->Set(svtkSelectionNode::INVERSE(), 0);

      if (hasCompositeKey)
      {
        outputProperties->Set(svtkSelectionNode::COMPOSITE_INDEX(), iter->GetCurrentFlatIndex());
      }

      if (hasHierarchicalKey && hbIter)
      {
        outputProperties->Set(svtkSelectionNode::HIERARCHICAL_LEVEL(), hbIter->GetCurrentLevel());
        outputProperties->Set(svtkSelectionNode::HIERARCHICAL_INDEX(), hbIter->GetCurrentIndex());
      }

      // Create a list of ids to select
      svtkSignedCharArray* insidedness = nullptr;

      int type = inputNode->GetFieldType();
      if (type == svtkSelectionNode::CELL)
      {
        insidedness = svtkArrayDownCast<svtkSignedCharArray>(
          dataset->GetCellData()->GetAbstractArray("svtkInsidedness"));
      }
      else if (type == svtkSelectionNode::POINT)
      {
        insidedness = svtkArrayDownCast<svtkSignedCharArray>(
          dataset->GetPointData()->GetAbstractArray("svtkInsidedness"));
      }
      else
      {
        svtkErrorMacro("Unknown field type");
        return 0;
      }

      assert(insidedness);

      // Convert the insidedness array into an index input.
      svtkNew<svtkIdTypeArray> idList;
      for (svtkIdType i = 0; i < insidedness->GetNumberOfTuples(); i++)
      {
        if (insidedness->GetValue(i) == 1)
        {
          idList->InsertNextValue(i);
        }
      }

      outputNode->SetSelectionList(idList);
      output->Union(outputNode);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkConvertSelection::Convert(svtkSelection* input, svtkDataObject* data, svtkSelection* output)
{
  for (unsigned int n = 0; n < input->GetNumberOfNodes(); ++n)
  {
    svtkSelectionNode* inputNode = input->GetNode(n);
    svtkSmartPointer<svtkSelectionNode> outputNode = svtkSmartPointer<svtkSelectionNode>::New();

    outputNode->ShallowCopy(inputNode);
    outputNode->SetContentType(this->OutputType);

    // If it is the same type, we are done
    if (inputNode->GetContentType() != svtkSelectionNode::VALUES &&
      inputNode->GetContentType() != svtkSelectionNode::THRESHOLDS &&
      inputNode->GetContentType() == this->OutputType)
    {
      output->Union(outputNode);
      continue;
    }

    // If the input is a values or thresholds selection, we need array names
    // on the selection arrays to perform the selection.
    if (inputNode->GetContentType() == svtkSelectionNode::VALUES ||
      inputNode->GetContentType() == svtkSelectionNode::THRESHOLDS)
    {
      svtkFieldData* selData = inputNode->GetSelectionData();
      for (int i = 0; i < selData->GetNumberOfArrays(); i++)
      {
        if (!selData->GetAbstractArray(i)->GetName())
        {
          svtkErrorMacro("Array name must be specified for values or thresholds selection.");
          return 0;
        }
      }
    }

    // If the output is a threshold selection, we need exactly one array name.
    if (this->OutputType == svtkSelectionNode::THRESHOLDS &&
      (this->ArrayNames == nullptr || this->ArrayNames->GetNumberOfValues() != 1))
    {
      svtkErrorMacro("One array name must be specified for thresholds selection.");
      return 0;
    }

    // If the output is a values selection, we need at lease one array name.
    if (this->OutputType == svtkSelectionNode::VALUES &&
      (this->ArrayNames == nullptr || this->ArrayNames->GetNumberOfValues() == 0))
    {
      svtkErrorMacro("At least one array name must be specified for values selection.");
      return 0;
    }

    // If we are converting a thresholds or values selection to
    // a selection on the same arrays, we are done.
    if ((inputNode->GetContentType() == svtkSelectionNode::VALUES ||
          inputNode->GetContentType() == svtkSelectionNode::THRESHOLDS) &&
      this->OutputType == inputNode->GetContentType() &&
      this->ArrayNames->GetNumberOfValues() == inputNode->GetSelectionData()->GetNumberOfArrays())
    {
      bool same = true;
      svtkFieldData* selData = inputNode->GetSelectionData();
      for (int i = 0; i < selData->GetNumberOfArrays(); i++)
      {
        if (strcmp(selData->GetAbstractArray(i)->GetName(), this->ArrayNames->GetValue(i)))
        {
          same = false;
          break;
        }
      }
      if (same)
      {
        output->Union(outputNode);
        continue;
      }
    }

    // Check whether we can do the conversion
    if (this->OutputType != svtkSelectionNode::VALUES &&
      this->OutputType != svtkSelectionNode::GLOBALIDS &&
      this->OutputType != svtkSelectionNode::PEDIGREEIDS &&
      this->OutputType != svtkSelectionNode::INDICES)
    {
      svtkErrorMacro("Cannot convert to type " << this->OutputType << " unless input type matches.");
      return 0;
    }

    // Get the correct field data
    svtkFieldData* fd = nullptr;
    svtkDataSetAttributes* dsa = nullptr;
    if (svtkDataSet::SafeDownCast(data))
    {
      if (!inputNode->GetProperties()->Has(svtkSelectionNode::FIELD_TYPE()) ||
        inputNode->GetFieldType() == svtkSelectionNode::CELL)
      {
        dsa = svtkDataSet::SafeDownCast(data)->GetCellData();
      }
      else if (inputNode->GetFieldType() == svtkSelectionNode::POINT)
      {
        dsa = svtkDataSet::SafeDownCast(data)->GetPointData();
      }
      else if (inputNode->GetFieldType() == svtkSelectionNode::FIELD)
      {
        fd = data->GetFieldData();
      }
      else
      {
        svtkErrorMacro("Inappropriate selection type for a svtkDataSet");
        return 0;
      }
    }
    else if (svtkGraph::SafeDownCast(data))
    {
      if (!inputNode->GetProperties()->Has(svtkSelectionNode::FIELD_TYPE()) ||
        inputNode->GetFieldType() == svtkSelectionNode::EDGE)
      {
        dsa = svtkGraph::SafeDownCast(data)->GetEdgeData();
      }
      else if (inputNode->GetFieldType() == svtkSelectionNode::VERTEX)
      {
        dsa = svtkGraph::SafeDownCast(data)->GetVertexData();
      }
      else if (inputNode->GetFieldType() == svtkSelectionNode::FIELD)
      {
        fd = data->GetFieldData();
      }
      else
      {
        svtkErrorMacro("Inappropriate selection type for a svtkGraph");
        return 0;
      }
    }
    else if (svtkTable::SafeDownCast(data))
    {
      if (!inputNode->GetProperties()->Has(svtkSelectionNode::FIELD_TYPE()) ||
        inputNode->GetFieldType() != svtkSelectionNode::FIELD)
      {
        dsa = svtkTable::SafeDownCast(data)->GetRowData();
      }
      else
      {
        fd = data->GetFieldData();
      }
    }
    else
    {
      if (!inputNode->GetProperties()->Has(svtkSelectionNode::FIELD_TYPE()) ||
        inputNode->GetFieldType() == svtkSelectionNode::FIELD)
      {
        fd = data->GetFieldData();
      }
      else
      {
        svtkErrorMacro("Inappropriate selection type for a non-dataset, non-graph");
        return 0;
      }
    }

    //
    // First, convert the selection to a list of indices
    //

    svtkSmartPointer<svtkIdTypeArray> indices = svtkSmartPointer<svtkIdTypeArray>::New();

    if (inputNode->GetContentType() == svtkSelectionNode::FRUSTUM ||
      inputNode->GetContentType() == svtkSelectionNode::LOCATIONS ||
      inputNode->GetContentType() == svtkSelectionNode::QUERY)
    {
      if (!svtkDataSet::SafeDownCast(data))
      {
        svtkErrorMacro(
          "Can only convert from frustum, locations, or query if the input is a svtkDataSet");
        return 0;
      }
      // Use the extract selection filter to create an index selection
      svtkSmartPointer<svtkSelectionNode> indexNode = svtkSmartPointer<svtkSelectionNode>::New();
      this->ConvertToIndexSelection(inputNode, svtkDataSet::SafeDownCast(data), indexNode);
      // TODO: We should shallow copy this, but the method is not defined.
      indices->DeepCopy(indexNode->GetSelectionList());
    }
    else if (inputNode->GetContentType() == svtkSelectionNode::THRESHOLDS)
    {
      svtkDoubleArray* lims = svtkArrayDownCast<svtkDoubleArray>(inputNode->GetSelectionList());
      if (!lims)
      {
        svtkErrorMacro("Thresholds selection requires svtkDoubleArray selection list.");
        return 0;
      }
      svtkDataArray* dataArr = nullptr;
      if (dsa)
      {
        dataArr = svtkArrayDownCast<svtkDataArray>(dsa->GetAbstractArray(lims->GetName()));
      }
      else if (fd)
      {
        dataArr = svtkArrayDownCast<svtkDataArray>(fd->GetAbstractArray(lims->GetName()));
      }
      if (!dataArr)
      {
        if (!this->AllowMissingArray)
        {
          svtkErrorMacro("Could not find svtkDataArray for thresholds selection.");
          return 0;
        }
        else
        {
          return 1;
        }
      }
      for (svtkIdType id = 0; id < dataArr->GetNumberOfTuples(); id++)
      {
        int keepPoint = svtkExtractSelectedThresholds::EvaluateValue(dataArr, id, lims);
        if (keepPoint)
        {
          indices->InsertNextValue(id);
        }
      }
    }
    else if (inputNode->GetContentType() == svtkSelectionNode::INDICES)
    {
      // TODO: We should shallow copy this, but the method is not defined.
      indices->DeepCopy(inputNode->GetSelectionList());
    }
    else if (inputNode->GetContentType() == svtkSelectionNode::VALUES)
    {
      svtkFieldData* selData = inputNode->GetSelectionData();
      svtkSmartPointer<svtkTable> selTable = svtkSmartPointer<svtkTable>::New();
      selTable->GetRowData()->ShallowCopy(selData);
      svtkSmartPointer<svtkTable> dataTable = svtkSmartPointer<svtkTable>::New();
      for (svtkIdType col = 0; col < selTable->GetNumberOfColumns(); col++)
      {
        svtkAbstractArray* dataArr = nullptr;
        if (dsa)
        {
          dataArr = dsa->GetAbstractArray(selTable->GetColumn(col)->GetName());
        }
        else if (fd)
        {
          dataArr = fd->GetAbstractArray(selTable->GetColumn(col)->GetName());
        }
        if (dataArr)
        {
          dataTable->AddColumn(dataArr);
        }
      }
      // Select rows matching selTable from the input dataTable
      // and put the matches in the index array.
      this->SelectTableFromTable(selTable, dataTable, indices);
    }
    else if (inputNode->GetContentType() == svtkSelectionNode::PEDIGREEIDS ||
      inputNode->GetContentType() == svtkSelectionNode::GLOBALIDS)
    {
      // Get the appropriate array
      svtkAbstractArray* selArr = inputNode->GetSelectionList();
      svtkAbstractArray* dataArr = nullptr;
      if (dsa && inputNode->GetContentType() == svtkSelectionNode::PEDIGREEIDS)
      {
        dataArr = dsa->GetPedigreeIds();
      }
      else if (dsa && inputNode->GetContentType() == svtkSelectionNode::GLOBALIDS)
      {
        dataArr = dsa->GetGlobalIds();
      }
      else if (fd && selArr->GetName())
      {
        // Since data objects only have field data which does not have attributes,
        // use the array name to try to match the incoming selection's array.
        dataArr = fd->GetAbstractArray(selArr->GetName());
      }
      else
      {
        svtkErrorMacro("Tried to use array name to match global or pedigree ids on data object,"
          << "but name not set on selection array.");
        return 0;
      }

      // Check array compatibility
      if (!dataArr)
      {
        if (!this->AllowMissingArray)
        {
          svtkErrorMacro("Selection array does not exist in input dataset.");
          return 0;
        }
        else
        {
          return 1;
        }
      }

      // Handle the special case where we have a domain array.
      svtkStringArray* domainArr =
        dsa ? svtkArrayDownCast<svtkStringArray>(dsa->GetAbstractArray("domain")) : nullptr;
      if (inputNode->GetContentType() == svtkSelectionNode::PEDIGREEIDS && domainArr &&
        selArr->GetName())
      {
        // Perform the lookup, keeping only those items in the correct domain.
        svtkStdString domain = selArr->GetName();
        svtkIdType numTuples = selArr->GetNumberOfTuples();
        svtkSmartPointer<svtkIdList> list = svtkSmartPointer<svtkIdList>::New();
        for (svtkIdType i = 0; i < numTuples; i++)
        {
          dataArr->LookupValue(selArr->GetVariantValue(i), list);
          svtkIdType numIds = list->GetNumberOfIds();
          for (svtkIdType j = 0; j < numIds; j++)
          {
            if (domainArr->GetValue(list->GetId(j)) == domain)
            {
              indices->InsertNextValue(list->GetId(j));
            }
          }
        }
      }
      // If no domain array, the name of the selection and data arrays
      // must match (if they exist).
      else if (inputNode->GetContentType() != svtkSelectionNode::PEDIGREEIDS || !selArr->GetName() ||
        !dataArr->GetName() || !strcmp(selArr->GetName(), dataArr->GetName()))
      {
        // Perform the lookup
        svtkIdType numTuples = selArr->GetNumberOfTuples();
        svtkSmartPointer<svtkIdList> list = svtkSmartPointer<svtkIdList>::New();
        for (svtkIdType i = 0; i < numTuples; i++)
        {
          dataArr->LookupValue(selArr->GetVariantValue(i), list);
          svtkIdType numIds = list->GetNumberOfIds();
          for (svtkIdType j = 0; j < numIds; j++)
          {
            indices->InsertNextValue(list->GetId(j));
          }
        }
      }
    }

    double progress = 0.8;
    this->InvokeEvent(svtkCommand::ProgressEvent, &progress);

    //
    // Now that we have the list of indices, convert the selection by indexing
    // values in another array.
    //

    // If it is an index selection, we are done.
    if (this->OutputType == svtkSelectionNode::INDICES)
    {
      outputNode->SetSelectionList(indices);
      output->Union(outputNode);
      continue;
    }

    svtkIdType numOutputArrays = 1;
    if (this->OutputType == svtkSelectionNode::VALUES)
    {
      numOutputArrays = this->ArrayNames->GetNumberOfValues();
    }

    // Handle the special case where we have a pedigree id selection with a domain array.
    svtkStringArray* outputDomainArr =
      dsa ? svtkArrayDownCast<svtkStringArray>(dsa->GetAbstractArray("domain")) : nullptr;
    if (this->OutputType == svtkSelectionNode::PEDIGREEIDS && outputDomainArr)
    {
      svtkAbstractArray* outputDataArr = dsa->GetPedigreeIds();
      // Check array existence.
      if (!outputDataArr)
      {
        if (!this->AllowMissingArray)
        {
          svtkErrorMacro("Output selection array does not exist in input dataset.");
          return 0;
        }
        else
        {
          return 1;
        }
      }

      std::map<svtkStdString, svtkSmartPointer<svtkAbstractArray> > domainArrays;
      svtkIdType numTuples = outputDataArr->GetNumberOfTuples();
      svtkIdType numIndices = indices->GetNumberOfTuples();
      for (svtkIdType i = 0; i < numIndices; ++i)
      {
        svtkIdType index = indices->GetValue(i);
        if (index >= numTuples)
        {
          continue;
        }
        svtkStdString domain = outputDomainArr->GetValue(index);
        if (domainArrays.count(domain) == 0)
        {
          domainArrays[domain].TakeReference(
            svtkAbstractArray::CreateArray(outputDataArr->GetDataType()));
          domainArrays[domain]->SetName(domain);
        }
        svtkAbstractArray* domainArr = domainArrays[domain];
        domainArr->InsertNextTuple(index, outputDataArr);
        if (i % 1000 == 0)
        {
          progress = 0.8 + (0.2 * i / numIndices);
          this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
        }
      }
      std::map<svtkStdString, svtkSmartPointer<svtkAbstractArray> >::iterator it, itEnd;
      it = domainArrays.begin();
      itEnd = domainArrays.end();
      for (; it != itEnd; ++it)
      {
        svtkSmartPointer<svtkSelectionNode> node = svtkSmartPointer<svtkSelectionNode>::New();
        node->SetContentType(svtkSelectionNode::PEDIGREEIDS);
        node->SetFieldType(inputNode->GetFieldType());
        node->SetSelectionList(it->second);
        output->Union(node);
      }
      continue;
    }

    svtkSmartPointer<svtkDataSetAttributes> outputData = svtkSmartPointer<svtkDataSetAttributes>::New();
    for (svtkIdType ind = 0; ind < numOutputArrays; ind++)
    {
      // Find the output array where to get the output selection values.
      svtkAbstractArray* outputDataArr = nullptr;
      if (dsa && this->OutputType == svtkSelectionNode::VALUES)
      {
        outputDataArr = dsa->GetAbstractArray(this->ArrayNames->GetValue(ind));
      }
      else if (fd && this->OutputType == svtkSelectionNode::VALUES)
      {
        outputDataArr = fd->GetAbstractArray(this->ArrayNames->GetValue(ind));
      }
      else if (dsa && this->OutputType == svtkSelectionNode::PEDIGREEIDS)
      {
        outputDataArr = dsa->GetPedigreeIds();
      }
      else if (dsa && this->OutputType == svtkSelectionNode::GLOBALIDS)
      {
        outputDataArr = dsa->GetGlobalIds();
      }
      else
      {
        // TODO: Make this error go away.
        svtkErrorMacro(
          "BUG: Currently you can only specify pedigree and global ids on a svtkDataSet.");
        return 0;
      }

      // Check array existence.
      if (outputDataArr)
      {
        // Put the array's values into the selection.
        svtkAbstractArray* outputArr = svtkAbstractArray::CreateArray(outputDataArr->GetDataType());
        outputArr->SetName(outputDataArr->GetName());
        svtkIdType numTuples = outputDataArr->GetNumberOfTuples();
        svtkIdType numIndices = indices->GetNumberOfTuples();
        for (svtkIdType i = 0; i < numIndices; ++i)
        {
          svtkIdType index = indices->GetValue(i);
          if (index < numTuples)
          {
            outputArr->InsertNextTuple(index, outputDataArr);
          }
          if (i % 1000 == 0)
          {
            progress = 0.8 + (0.2 * (ind * numIndices + i)) / (numOutputArrays * numIndices);
            this->InvokeEvent(svtkCommand::ProgressEvent, &progress);
          }
        }

        if (this->MatchAnyValues)
        {
          svtkSmartPointer<svtkSelectionNode> outNode = svtkSmartPointer<svtkSelectionNode>::New();
          outNode->ShallowCopy(inputNode);
          outNode->SetContentType(this->OutputType);
          outNode->SetSelectionList(outputArr);
          output->AddNode(outNode);
        }
        else
        {
          outputData->AddArray(outputArr);
        }

        outputArr->Delete();
      }
    }

    // If there are no output arrays, just add a dummy one so
    // that the selection list is not null.
    if (outputData->GetNumberOfArrays() == 0)
    {
      svtkSmartPointer<svtkIdTypeArray> arr = svtkSmartPointer<svtkIdTypeArray>::New();
      arr->SetName("Empty");
      outputData->AddArray(arr);
    }

    outputNode->SetSelectionData(outputData);
    output->Union(outputNode);
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkConvertSelection::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkSelection* origInput = svtkSelection::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!this->SelectionExtractor)
  {
    svtkNew<svtkExtractSelection> se;
    this->SetSelectionExtractor(se);
  }

  svtkSmartPointer<svtkSelection> input = svtkSmartPointer<svtkSelection>::New();
  input->ShallowCopy(origInput);
  if (this->InputFieldType != -1)
  {
    for (unsigned int i = 0; i < input->GetNumberOfNodes(); ++i)
    {
      input->GetNode(i)->SetFieldType(this->InputFieldType);
    }
  }

  svtkInformation* dataInfo = inputVector[1]->GetInformationObject(0);
  svtkDataObject* data = dataInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkSelection* output = svtkSelection::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (data && data->IsA("svtkCompositeDataSet"))
  {
    return this->ConvertCompositeDataSet(input, static_cast<svtkCompositeDataSet*>(data), output);
  }

  return this->Convert(input, data, output);
}

//----------------------------------------------------------------------------
void svtkConvertSelection::SetDataObjectConnection(svtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);
}

//----------------------------------------------------------------------------
int svtkConvertSelection::FillInputPortInformation(int port, svtkInformation* info)
{
  // now add our info
  if (port == 0)
  {
    info->Set(svtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
  }
  else if (port == 1)
  {
    // Can convert from a svtkDataSet, svtkGraph, or svtkTable
    info->Remove(svtkConvertSelection::INPUT_REQUIRED_DATA_TYPE());
    info->Append(svtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
    info->Append(svtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    info->Append(svtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    info->Append(svtkConvertSelection::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkConvertSelection::GetSelectedItems(
  svtkSelection* input, svtkDataObject* data, int fieldType, svtkIdTypeArray* indices)
{
  svtkSelection* indexSel =
    svtkConvertSelection::ToSelectionType(input, data, svtkSelectionNode::INDICES);
  for (unsigned int n = 0; n < indexSel->GetNumberOfNodes(); ++n)
  {
    svtkSelectionNode* node = indexSel->GetNode(n);
    svtkIdTypeArray* list = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());
    if (node->GetFieldType() == fieldType && node->GetContentType() == svtkSelectionNode::INDICES &&
      list)
    {
      for (svtkIdType i = 0; i < list->GetNumberOfTuples(); ++i)
      {
        svtkIdType cur = list->GetValue(i);
        if (indices->LookupValue(cur) < 0)
        {
          indices->InsertNextValue(cur);
        }
      }
    }
  }
  indexSel->Delete();
}

//----------------------------------------------------------------------------
void svtkConvertSelection::GetSelectedVertices(
  svtkSelection* input, svtkGraph* data, svtkIdTypeArray* indices)
{
  svtkConvertSelection::GetSelectedItems(input, data, svtkSelectionNode::VERTEX, indices);
}

//----------------------------------------------------------------------------
void svtkConvertSelection::GetSelectedEdges(
  svtkSelection* input, svtkGraph* data, svtkIdTypeArray* indices)
{
  svtkConvertSelection::GetSelectedItems(input, data, svtkSelectionNode::EDGE, indices);
}

//----------------------------------------------------------------------------
void svtkConvertSelection::GetSelectedPoints(
  svtkSelection* input, svtkDataSet* data, svtkIdTypeArray* indices)
{
  svtkConvertSelection::GetSelectedItems(input, data, svtkSelectionNode::POINT, indices);
}

//----------------------------------------------------------------------------
void svtkConvertSelection::GetSelectedCells(
  svtkSelection* input, svtkDataSet* data, svtkIdTypeArray* indices)
{
  svtkConvertSelection::GetSelectedItems(input, data, svtkSelectionNode::CELL, indices);
}

//----------------------------------------------------------------------------
void svtkConvertSelection::GetSelectedRows(
  svtkSelection* input, svtkTable* data, svtkIdTypeArray* indices)
{
  svtkConvertSelection::GetSelectedItems(input, data, svtkSelectionNode::ROW, indices);
}

//----------------------------------------------------------------------------
svtkSelection* svtkConvertSelection::ToIndexSelection(svtkSelection* input, svtkDataObject* data)
{
  return svtkConvertSelection::ToSelectionType(input, data, svtkSelectionNode::INDICES);
}

//----------------------------------------------------------------------------
svtkSelection* svtkConvertSelection::ToGlobalIdSelection(svtkSelection* input, svtkDataObject* data)
{
  return svtkConvertSelection::ToSelectionType(input, data, svtkSelectionNode::GLOBALIDS);
}

//----------------------------------------------------------------------------
svtkSelection* svtkConvertSelection::ToPedigreeIdSelection(svtkSelection* input, svtkDataObject* data)
{
  return svtkConvertSelection::ToSelectionType(input, data, svtkSelectionNode::PEDIGREEIDS);
}

//----------------------------------------------------------------------------
svtkSelection* svtkConvertSelection::ToValueSelection(
  svtkSelection* input, svtkDataObject* data, const char* arrayName)
{
  SVTK_CREATE(svtkStringArray, names);
  names->InsertNextValue(arrayName);
  return svtkConvertSelection::ToSelectionType(input, data, svtkSelectionNode::VALUES, names);
}

//----------------------------------------------------------------------------
svtkSelection* svtkConvertSelection::ToValueSelection(
  svtkSelection* input, svtkDataObject* data, svtkStringArray* arrayNames)
{
  return svtkConvertSelection::ToSelectionType(input, data, svtkSelectionNode::VALUES, arrayNames);
}

//----------------------------------------------------------------------------
svtkSelection* svtkConvertSelection::ToSelectionType(svtkSelection* input, svtkDataObject* data,
  int type, svtkStringArray* arrayNames, int inputFieldType, bool allowMissingArray)
{
  SVTK_CREATE(svtkConvertSelection, convert);
  svtkDataObject* dataCopy = data->NewInstance();
  dataCopy->ShallowCopy(data);
  SVTK_CREATE(svtkSelection, inputCopy);
  inputCopy->ShallowCopy(input);
  convert->SetInputData(0, inputCopy);
  convert->SetInputData(1, dataCopy);
  convert->SetOutputType(type);
  convert->SetArrayNames(arrayNames);
  convert->SetInputFieldType(inputFieldType);
  convert->SetAllowMissingArray(allowMissingArray);
  convert->Update();
  svtkSelection* output = convert->GetOutput();
  output->Register(nullptr);
  dataCopy->Delete();
  return output;
}

//----------------------------------------------------------------------------
void svtkConvertSelection::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InputFieldType: " << this->InputFieldType << endl;
  os << indent << "OutputType: " << this->OutputType << endl;
  os << indent << "SelectionExtractor: " << this->SelectionExtractor << endl;
  os << indent << "MatchAnyValues: " << (this->MatchAnyValues ? "true" : "false") << endl;
  os << indent << "AllowMissingArray: " << (this->AllowMissingArray ? "true" : "false") << endl;
  os << indent << "ArrayNames: " << (this->ArrayNames ? "" : "(null)") << endl;
  if (this->ArrayNames)
  {
    this->ArrayNames->PrintSelf(os, indent.GetNextIndent());
  }
}
