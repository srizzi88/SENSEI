/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAddMembershipArray.cxx

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

#include "svtkAddMembershipArray.h"

#include "svtkAbstractArray.h"
#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkBitArray.h"
#include "svtkCellData.h"
#include "svtkConvertSelection.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkVariant.h"
#include "svtkVariantArray.h"

svtkStandardNewMacro(svtkAddMembershipArray);
svtkCxxSetObjectMacro(svtkAddMembershipArray, InputValues, svtkAbstractArray);

//---------------------------------------------------------------------------
svtkAddMembershipArray::svtkAddMembershipArray()
{
  this->FieldType = -1;
  this->OutputArrayName = nullptr;
  this->SetOutputArrayName("membership");
  this->InputArrayName = nullptr;
  this->InputValues = nullptr;
  this->SetNumberOfInputPorts(3);
}

//---------------------------------------------------------------------------
svtkAddMembershipArray::~svtkAddMembershipArray()
{
  this->SetOutputArrayName(nullptr);
  this->SetInputArrayName(nullptr);
}

//---------------------------------------------------------------------------
int svtkAddMembershipArray::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkTable");
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkSelection");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  else if (port == 2)
  {
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkAnnotationLayers");
    return 1;
  }

  return 1;
}

//---------------------------------------------------------------------------
int svtkAddMembershipArray::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkSelection* inputSelection = svtkSelection::GetData(inputVector[1]);
  svtkAnnotationLayers* inputAnnotations = svtkAnnotationLayers::GetData(inputVector[2]);
  svtkInformation* outputInfo = outputVector->GetInformationObject(0);
  svtkDataObject* output = outputInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkGraph* graph = svtkGraph::SafeDownCast(output);
  svtkTable* table = svtkTable::SafeDownCast(output);

  output->ShallowCopy(input);

  if (!inputSelection)
  {
    if (!this->InputArrayName || !this->InputValues)
      return 1;

    svtkDataSetAttributes* ds = nullptr;
    switch (this->FieldType)
    {
      case svtkAddMembershipArray::VERTEX_DATA:
        if (graph)
        {
          ds = graph->GetVertexData();
        }
        break;
      case svtkAddMembershipArray::EDGE_DATA:
        if (graph)
        {
          ds = graph->GetEdgeData();
        }
        break;
      case svtkAddMembershipArray::ROW_DATA:
        if (table)
        {
          ds = table->GetRowData();
        }
        break;
    }

    if (!ds)
    {
      svtkErrorMacro("Unsupported input field type.");
      return 0;
    }

    svtkIntArray* vals = svtkIntArray::New();
    vals->SetNumberOfTuples(ds->GetNumberOfTuples());
    vals->SetNumberOfComponents(1);
    vals->SetName(this->OutputArrayName);
    vals->FillComponent(0, 0);

    svtkAbstractArray* inputArray = ds->GetAbstractArray(this->InputArrayName);
    if (inputArray && this->InputValues)
    {
      for (svtkIdType i = 0; i < inputArray->GetNumberOfTuples(); ++i)
      {
        svtkVariant v(0);
        switch (inputArray->GetDataType())
        {
          svtkExtraExtendedTemplateMacro(v = *static_cast<SVTK_TT*>(inputArray->GetVoidPointer(i)));
        }
        if (this->InputValues->LookupValue(v) >= 0)
        {
          vals->SetValue(i, 1);
        }
        else
        {
          vals->SetValue(i, 0);
        }
      }
    }

    ds->AddArray(vals);

    vals->Delete();

    return 1;
  }

  svtkSmartPointer<svtkSelection> selection = svtkSmartPointer<svtkSelection>::New();
  selection->DeepCopy(inputSelection);

  if (inputAnnotations)
  {
    for (unsigned int i = 0; i < inputAnnotations->GetNumberOfAnnotations(); ++i)
    {
      svtkAnnotation* a = inputAnnotations->GetAnnotation(i);
      if (a->GetInformation()->Has(svtkAnnotation::ENABLE()) &&
        a->GetInformation()->Get(svtkAnnotation::ENABLE()) == 0)
      {
        continue;
      }
      selection->Union(a->GetSelection());
    }
  }

  svtkSmartPointer<svtkIdTypeArray> rowList = svtkSmartPointer<svtkIdTypeArray>::New();
  svtkSmartPointer<svtkIdTypeArray> edgeList = svtkSmartPointer<svtkIdTypeArray>::New();
  svtkSmartPointer<svtkIdTypeArray> vertexList = svtkSmartPointer<svtkIdTypeArray>::New();

  if (graph)
  {
    svtkConvertSelection::GetSelectedVertices(selection, graph, vertexList);
    svtkConvertSelection::GetSelectedEdges(selection, graph, edgeList);
  }
  else if (table)
  {
    svtkConvertSelection::GetSelectedRows(selection, table, rowList);
  }

  if (vertexList->GetNumberOfTuples() != 0)
  {
    svtkSmartPointer<svtkIntArray> vals = svtkSmartPointer<svtkIntArray>::New();
    vals->SetNumberOfTuples(graph->GetVertexData()->GetNumberOfTuples());
    vals->SetNumberOfComponents(1);
    vals->SetName(this->OutputArrayName);
    vals->FillComponent(0, 0);
    svtkIdType numSelectedVerts = vertexList->GetNumberOfTuples();
    for (svtkIdType i = 0; i < numSelectedVerts; ++i)
    {
      vals->SetValue(vertexList->GetValue(i), 1);
    }
    graph->GetVertexData()->AddArray(vals);
  }

  if (edgeList->GetNumberOfTuples() != 0)
  {
    svtkSmartPointer<svtkIntArray> vals = svtkSmartPointer<svtkIntArray>::New();
    vals->SetNumberOfTuples(graph->GetEdgeData()->GetNumberOfTuples());
    vals->SetNumberOfComponents(1);
    vals->SetName(this->OutputArrayName);
    vals->FillComponent(0, 0);
    svtkIdType numSelectedEdges = edgeList->GetNumberOfTuples();
    for (svtkIdType i = 0; i < numSelectedEdges; ++i)
    {
      vals->SetValue(edgeList->GetValue(i), 1);
    }
    graph->GetEdgeData()->AddArray(vals);
  }

  if (rowList->GetNumberOfTuples() != 0)
  {
    svtkSmartPointer<svtkIntArray> vals = svtkSmartPointer<svtkIntArray>::New();
    vals->SetNumberOfTuples(table->GetRowData()->GetNumberOfTuples());
    vals->SetNumberOfComponents(1);
    vals->SetName(this->OutputArrayName);
    vals->FillComponent(0, 0);
    svtkIdType numSelectedRows = rowList->GetNumberOfTuples();
    for (svtkIdType i = 0; i < numSelectedRows; ++i)
    {
      vals->SetValue(rowList->GetValue(i), 1);
    }
    table->GetRowData()->AddArray(vals);
  }

  return 1;
}

//---------------------------------------------------------------------------
void svtkAddMembershipArray::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldType: " << this->FieldType << endl;
  os << indent << "OutputArrayName: " << (this->OutputArrayName ? this->OutputArrayName : "(none)")
     << endl;
  os << indent << "InputArrayName: " << (this->InputArrayName ? this->InputArrayName : "(none)")
     << endl;
  if (this->InputValues)
  {
    os << indent << "Input Values :" << endl;
    int num = this->InputValues->GetNumberOfTuples();
    for (int idx = 0; idx < num; ++idx)
    {
      svtkVariant v(0);
      switch (this->InputValues->GetDataType())
      {
        svtkExtraExtendedTemplateMacro(
          v = *static_cast<SVTK_TT*>(this->InputValues->GetVoidPointer(idx)));
      }
      os << v.ToString() << endl;
    }
  }
}
