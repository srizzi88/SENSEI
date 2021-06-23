/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkConvertSelectionDomain.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkConvertSelectionDomain.h"

#include "svtkAbstractArray.h"
#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkGraph.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

#include <set>

svtkStandardNewMacro(svtkConvertSelectionDomain);
//----------------------------------------------------------------------------
svtkConvertSelectionDomain::svtkConvertSelectionDomain()
{
  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(2);
}

//----------------------------------------------------------------------------
svtkConvertSelectionDomain::~svtkConvertSelectionDomain() = default;

//----------------------------------------------------------------------------
static void svtkConvertSelectionDomainFindDomains(
  svtkDataSetAttributes* dsa, std::set<svtkStdString>& domains)
{
  if (dsa->GetAbstractArray("domain"))
  {
    svtkStringArray* domainArr = svtkArrayDownCast<svtkStringArray>(dsa->GetAbstractArray("domain"));
    if (!domainArr)
    {
      return; // Do nothing if the array isn't a string array
    }
    svtkIdType numTuples = domainArr->GetNumberOfTuples();
    for (svtkIdType i = 0; i < numTuples; ++i)
    {
      svtkStdString d = domainArr->GetValue(i);
      if (domains.count(d) == 0)
      {
        domains.insert(d);
      }
    }
  }
  else if (dsa->GetPedigreeIds() && dsa->GetPedigreeIds()->GetName())
  {
    domains.insert(dsa->GetPedigreeIds()->GetName());
  }
}

static void svtkConvertSelectionDomainConvertAnnotationDomain(svtkAnnotation* annIn,
  svtkAnnotation* annOut, std::set<svtkStdString>& domains1, std::set<svtkStdString>& domains2,
  svtkDataSetAttributes* dsa1, svtkDataSetAttributes* dsa2, int fieldType1, int fieldType2,
  svtkMultiBlockDataSet* maps)
{
  svtkSelection* inputSel = annIn->GetSelection();
  svtkSmartPointer<svtkSelection> outputSel = svtkSmartPointer<svtkSelection>::New();
  // Iterate over all input selections
  for (unsigned int c = 0; c < inputSel->GetNumberOfNodes(); ++c)
  {
    svtkSelectionNode* curInput = inputSel->GetNode(c);
    svtkSmartPointer<svtkSelectionNode> curOutput = svtkSmartPointer<svtkSelectionNode>::New();
    svtkAbstractArray* inArr = curInput->GetSelectionList();

    // Start with a shallow copy of the input selection.
    curOutput->ShallowCopy(curInput);

    // I don't know how to handle this type of selection,
    // so pass it through.
    if (!inArr || !inArr->GetName() || curInput->GetContentType() != svtkSelectionNode::PEDIGREEIDS)
    {
      outputSel->AddNode(curOutput);
      continue;
    }

    // If the selection already matches, we are done.
    if (domains1.count(inArr->GetName()) > 0)
    {
      curOutput->SetFieldType(fieldType1);
      outputSel->AddNode(curOutput);
      continue;
    }
    if (domains2.count(inArr->GetName()) > 0)
    {
      curOutput->SetFieldType(fieldType2);
      outputSel->AddNode(curOutput);
      continue;
    }

    // Select the correct source and destination mapping arrays.
    svtkAbstractArray* fromArr = nullptr;
    svtkAbstractArray* toArr = nullptr;
    unsigned int numMaps = maps->GetNumberOfBlocks();
    for (unsigned int i = 0; i < numMaps; ++i)
    {
      fromArr = nullptr;
      toArr = nullptr;
      svtkTable* table = svtkTable::SafeDownCast(maps->GetBlock(i));
      if (table)
      {
        fromArr = table->GetColumnByName(inArr->GetName());
        std::set<svtkStdString>::iterator it, itEnd;
        if (dsa1)
        {
          it = domains1.begin();
          itEnd = domains1.end();
          for (; it != itEnd; ++it)
          {
            toArr = table->GetColumnByName(it->c_str());
            if (toArr)
            {
              curOutput->SetFieldType(fieldType1);
              break;
            }
          }
        }
        if (!toArr && dsa2)
        {
          it = domains2.begin();
          itEnd = domains2.end();
          for (; it != itEnd; ++it)
          {
            toArr = table->GetColumnByName(it->c_str());
            if (toArr)
            {
              curOutput->SetFieldType(fieldType2);
              break;
            }
          }
        }
      }
      if (fromArr && toArr)
      {
        break;
      }
    }

    // Cannot do the conversion, so don't pass this selection
    // to the output.
    if (!fromArr || !toArr)
    {
      continue;
    }

    // Lookup values in the input selection and map them
    // through the table to the output selection.
    svtkIdType numTuples = inArr->GetNumberOfTuples();
    svtkSmartPointer<svtkAbstractArray> outArr;
    outArr.TakeReference(svtkAbstractArray::CreateArray(toArr->GetDataType()));
    outArr->SetName(toArr->GetName());
    svtkSmartPointer<svtkIdList> ids = svtkSmartPointer<svtkIdList>::New();
    for (svtkIdType i = 0; i < numTuples; ++i)
    {
      fromArr->LookupValue(inArr->GetVariantValue(i), ids);
      svtkIdType numIds = ids->GetNumberOfIds();
      for (svtkIdType j = 0; j < numIds; ++j)
      {
        outArr->InsertNextTuple(ids->GetId(j), toArr);
      }
    }
    curOutput->SetSelectionList(outArr);
    outputSel->AddNode(curOutput);
  }
  // Make sure there is at least something in the output selection.
  if (outputSel->GetNumberOfNodes() == 0)
  {
    svtkSmartPointer<svtkSelectionNode> node = svtkSmartPointer<svtkSelectionNode>::New();
    node->SetContentType(svtkSelectionNode::INDICES);
    svtkSmartPointer<svtkIdTypeArray> inds = svtkSmartPointer<svtkIdTypeArray>::New();
    node->SetSelectionList(inds);
    outputSel->AddNode(node);
  }

  annOut->ShallowCopy(annIn);
  annOut->SetSelection(outputSel);
}

//----------------------------------------------------------------------------
int svtkConvertSelectionDomain::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Retrieve the input and output.
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkAnnotationLayers* inputAnn = svtkAnnotationLayers::SafeDownCast(input);

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkAnnotationLayers* outputAnn = svtkAnnotationLayers::SafeDownCast(output);

  outInfo = outputVector->GetInformationObject(1);
  svtkSelection* outputCurrentSel =
    svtkSelection::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // If we have no mapping table, we are done.
  svtkInformation* mapInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* dataInfo = inputVector[2]->GetInformationObject(0);
  if (!dataInfo || !mapInfo)
  {
    output->ShallowCopy(input);
    return 1;
  }

  // If the input is instead a svtkSelection, wrap it in a svtkAnnotationLayers
  // object so it can be used uniformly in the function.
  bool createdInput = false;
  if (!inputAnn)
  {
    svtkSelection* inputSel = svtkSelection::SafeDownCast(input);
    inputAnn = svtkAnnotationLayers::New();
    inputAnn->SetCurrentSelection(inputSel);
    svtkSelection* outputSel = svtkSelection::SafeDownCast(output);
    outputAnn = svtkAnnotationLayers::New();
    outputAnn->SetCurrentSelection(outputSel);
    createdInput = true;
  }

  svtkMultiBlockDataSet* maps =
    svtkMultiBlockDataSet::SafeDownCast(mapInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataObject* data = dataInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkDataSetAttributes* dsa1 = nullptr;
  int fieldType1 = 0;
  svtkDataSetAttributes* dsa2 = nullptr;
  int fieldType2 = 0;
  if (svtkDataSet::SafeDownCast(data))
  {
    dsa1 = svtkDataSet::SafeDownCast(data)->GetPointData();
    fieldType1 = svtkSelectionNode::POINT;
    dsa2 = svtkDataSet::SafeDownCast(data)->GetCellData();
    fieldType2 = svtkSelectionNode::CELL;
  }
  else if (svtkGraph::SafeDownCast(data))
  {
    dsa1 = svtkGraph::SafeDownCast(data)->GetVertexData();
    fieldType1 = svtkSelectionNode::VERTEX;
    dsa2 = svtkGraph::SafeDownCast(data)->GetEdgeData();
    fieldType2 = svtkSelectionNode::EDGE;
  }
  else if (svtkTable::SafeDownCast(data))
  {
    dsa1 = svtkDataSetAttributes::SafeDownCast(svtkTable::SafeDownCast(data)->GetRowData());
    fieldType1 = svtkSelectionNode::ROW;
  }

  std::set<svtkStdString> domains1;
  std::set<svtkStdString> domains2;
  if (dsa1)
  {
    svtkConvertSelectionDomainFindDomains(dsa1, domains1);
  }
  if (dsa2)
  {
    svtkConvertSelectionDomainFindDomains(dsa2, domains2);
  }

  for (unsigned int a = 0; a < inputAnn->GetNumberOfAnnotations(); ++a)
  {
    svtkSmartPointer<svtkAnnotation> ann = svtkSmartPointer<svtkAnnotation>::New();
    svtkConvertSelectionDomainConvertAnnotationDomain(inputAnn->GetAnnotation(a), ann, domains1,
      domains2, dsa1, dsa2, fieldType1, fieldType2, maps);
    outputAnn->AddAnnotation(ann);
  }

  if (inputAnn->GetCurrentAnnotation())
  {
    svtkSmartPointer<svtkAnnotation> ann = svtkSmartPointer<svtkAnnotation>::New();
    svtkConvertSelectionDomainConvertAnnotationDomain(inputAnn->GetCurrentAnnotation(), ann,
      domains1, domains2, dsa1, dsa2, fieldType1, fieldType2, maps);
    outputAnn->SetCurrentAnnotation(ann);
  }
  else
  {
    outputAnn->SetCurrentAnnotation(nullptr);
  }

  // Copy current selection to the second output
  if (outputAnn->GetCurrentSelection())
  {
    outputCurrentSel->ShallowCopy(outputAnn->GetCurrentSelection());
  }

  if (createdInput)
  {
    inputAnn->Delete();
    outputAnn->Delete();
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkConvertSelectionDomain::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkAnnotationLayers");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkMultiBlockDataSet");
  }
  else if (port == 2)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkConvertSelectionDomain::FillOutputPortInformation(int port, svtkInformation* info)
{
  this->Superclass::FillOutputPortInformation(port, info);
  if (port == 1)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkSelection");
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkConvertSelectionDomain::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
