/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedRows.cxx

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

#include "svtkExtractSelectedRows.h"

#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkArrayDispatch.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCommand.h"
#include "svtkConvertSelection.h"
#include "svtkDataArray.h"
#include "svtkDataArrayRange.h"
#include "svtkEdgeListIterator.h"
#include "svtkEventForwarderCommand.h"
#include "svtkExtractSelection.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSignedCharArray.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTree.h"
#include "svtkVertexListIterator.h"

#include <map>
#include <vector>

svtkStandardNewMacro(svtkExtractSelectedRows);
//----------------------------------------------------------------------------
svtkExtractSelectedRows::svtkExtractSelectedRows()
{
  this->AddOriginalRowIdsArray = false;
  this->SetNumberOfInputPorts(3);
}

//----------------------------------------------------------------------------
svtkExtractSelectedRows::~svtkExtractSelectedRows() = default;

//----------------------------------------------------------------------------
int svtkExtractSelectedRows::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    return 1;
  }
  else if (port == 2)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkAnnotationLayers");
    return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedRows::SetSelectionConnection(svtkAlgorithmOutput* in)
{
  this->SetInputConnection(1, in);
}

//----------------------------------------------------------------------------
void svtkExtractSelectedRows::SetAnnotationLayersConnection(svtkAlgorithmOutput* in)
{
  this->SetInputConnection(2, in);
}

namespace
{
struct svtkCopySelectedRows
{
  template <class ArrayT>
  void operator()(ArrayT* list, svtkTable* input, svtkTable* output, svtkIdTypeArray* originalRowIds,
    bool addOriginalRowIdsArray) const
  {
    for (auto value : svtk::DataArrayValueRange(list))
    {
      svtkIdType val = static_cast<svtkIdType>(value);
      output->InsertNextRow(input->GetRow(val));
      if (addOriginalRowIdsArray)
      {
        originalRowIds->InsertNextValue(val);
      }
    }
  }
};
} // namespace

//----------------------------------------------------------------------------
int svtkExtractSelectedRows::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkTable* input = svtkTable::GetData(inputVector[0]);
  svtkSelection* inputSelection = svtkSelection::GetData(inputVector[1]);
  svtkAnnotationLayers* inputAnnotations = svtkAnnotationLayers::GetData(inputVector[2]);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkTable* output = svtkTable::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!inputSelection && !inputAnnotations)
  {
    svtkErrorMacro("No svtkSelection or svtkAnnotationLayers provided as input.");
    return 0;
  }

  svtkSmartPointer<svtkSelection> selection = svtkSmartPointer<svtkSelection>::New();
  int numSelections = 0;
  if (inputSelection)
  {
    selection->DeepCopy(inputSelection);
    numSelections++;
  }

  // If input annotations are provided, extract their selections only if
  // they are enabled and not hidden.
  if (inputAnnotations)
  {
    for (unsigned int i = 0; i < inputAnnotations->GetNumberOfAnnotations(); ++i)
    {
      svtkAnnotation* a = inputAnnotations->GetAnnotation(i);
      if ((a->GetInformation()->Has(svtkAnnotation::ENABLE()) &&
            a->GetInformation()->Get(svtkAnnotation::ENABLE()) == 0) ||
        (a->GetInformation()->Has(svtkAnnotation::ENABLE()) &&
          a->GetInformation()->Get(svtkAnnotation::ENABLE()) == 1 &&
          a->GetInformation()->Has(svtkAnnotation::HIDE()) &&
          a->GetInformation()->Get(svtkAnnotation::HIDE()) == 1))
      {
        continue;
      }
      selection->Union(a->GetSelection());
      numSelections++;
    }
  }

  // Handle case where there was no input selection and no enabled, non-hidden
  // annotations
  if (numSelections == 0)
  {
    output->ShallowCopy(input);
    return 1;
  }

  // Convert the selection to an INDICES selection
  svtkSmartPointer<svtkSelection> converted;
  converted.TakeReference(svtkConvertSelection::ToSelectionType(
    selection, input, svtkSelectionNode::INDICES, nullptr, svtkSelectionNode::ROW));
  if (!converted)
  {
    svtkErrorMacro("Selection conversion to INDICES failed.");
    return 0;
  }

  svtkIdTypeArray* originalRowIds = svtkIdTypeArray::New();
  originalRowIds->SetName("svtkOriginalRowIds");

  output->GetRowData()->CopyStructure(input->GetRowData());

  for (unsigned int i = 0; i < converted->GetNumberOfNodes(); ++i)
  {
    svtkSelectionNode* node = converted->GetNode(i);
    if (node->GetFieldType() == svtkSelectionNode::ROW)
    {
      svtkDataArray* list = svtkDataArray::SafeDownCast(node->GetSelectionList());
      if (list)
      {
        int inverse = node->GetProperties()->Get(svtkSelectionNode::INVERSE());
        if (inverse)
        {
          svtkIdType numRows = input->GetNumberOfRows(); // How many rows are in the whole dataset
          for (svtkIdType j = 0; j < numRows; ++j)
          {
            if (list->LookupValue(j) < 0)
            {
              output->InsertNextRow(input->GetRow(j));
              if (this->AddOriginalRowIdsArray)
              {
                originalRowIds->InsertNextValue(j);
              }
            }
          }
        }
        else
        {
          if (list->GetNumberOfComponents() != 1)
          {
            svtkGenericWarningMacro("NumberOfComponents expected to be 1.");
          }

          using Dispatcher = svtkArrayDispatch::DispatchByValueType<svtkArrayDispatch::Integrals>;
          if (!Dispatcher::Execute(list, svtkCopySelectedRows{}, input, output, originalRowIds,
                this->AddOriginalRowIdsArray))
          { // fallback for unsupported array types and non-integral value types:
            svtkCopySelectedRows{}(
              list, input, output, originalRowIds, this->AddOriginalRowIdsArray);
          }
        }
      }
    }
  }
  if (this->AddOriginalRowIdsArray)
  {
    output->AddColumn(originalRowIds);
  }
  originalRowIds->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractSelectedRows::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AddOriginalRowIdsArray: " << this->AddOriginalRowIdsArray << endl;
}
