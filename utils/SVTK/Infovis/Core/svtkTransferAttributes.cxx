/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkTransferAttributes.cxx

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
#include "svtkTransferAttributes.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataObject.h"
#include "svtkDataSet.h"
#include "svtkEdgeListIterator.h"
#include "svtkFloatArray.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkTable.h"
#include "svtkTree.h"
#include "svtkVariantArray.h"

#include <map>

//---------------------------------------------------------------------------
template <typename T>
svtkVariant svtkGetValue(T* arr, svtkIdType index)
{
  return svtkVariant(arr[index]);
}

//---------------------------------------------------------------------------
static svtkVariant svtkGetVariantValue(svtkAbstractArray* arr, svtkIdType i)
{
  svtkVariant val;
  switch (arr->GetDataType())
  {
    svtkExtraExtendedTemplateMacro(
      val = svtkGetValue(static_cast<SVTK_TT*>(arr->GetVoidPointer(0)), i));
  }
  return val;
}

svtkStandardNewMacro(svtkTransferAttributes);

svtkTransferAttributes::svtkTransferAttributes()
{
  this->SetNumberOfInputPorts(2);
  this->DirectMapping = false;
  this->DefaultValue = 1;
  this->SourceArrayName = nullptr;
  this->TargetArrayName = nullptr;
  this->SourceFieldType = svtkDataObject::FIELD_ASSOCIATION_POINTS;
  this->TargetFieldType = svtkDataObject::FIELD_ASSOCIATION_POINTS;
}

svtkTransferAttributes::~svtkTransferAttributes()
{
  this->SetSourceArrayName(nullptr);
  this->SetTargetArrayName(nullptr);
}

int svtkTransferAttributes::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
    return 1;
  }
  else if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
    return 1;
  }
  return 0;
}

int svtkTransferAttributes::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* targetInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDataObject* sourceInput = sourceInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* targetInput = targetInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  output->ShallowCopy(targetInput);

  // get the input and output
  int item_count_source = 0;
  svtkDataSetAttributes* dsa_source = nullptr;
  if (svtkDataSet::SafeDownCast(sourceInput) &&
    this->SourceFieldType == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    dsa_source = svtkDataSet::SafeDownCast(sourceInput)->GetPointData();
    item_count_source = svtkDataSet::SafeDownCast(sourceInput)->GetNumberOfPoints();
  }
  else if (svtkDataSet::SafeDownCast(sourceInput) &&
    this->SourceFieldType == svtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    dsa_source = svtkDataSet::SafeDownCast(sourceInput)->GetCellData();
    item_count_source = svtkDataSet::SafeDownCast(sourceInput)->GetNumberOfCells();
  }
  else if (svtkGraph::SafeDownCast(sourceInput) &&
    this->SourceFieldType == svtkDataObject::FIELD_ASSOCIATION_VERTICES)
  {
    dsa_source = svtkGraph::SafeDownCast(sourceInput)->GetVertexData();
    item_count_source = svtkGraph::SafeDownCast(sourceInput)->GetNumberOfVertices();
  }
  else if (svtkGraph::SafeDownCast(sourceInput) &&
    this->SourceFieldType == svtkDataObject::FIELD_ASSOCIATION_EDGES)
  {
    dsa_source = svtkGraph::SafeDownCast(sourceInput)->GetEdgeData();
    item_count_source = svtkGraph::SafeDownCast(sourceInput)->GetNumberOfEdges();
  }
  else if (svtkTable::SafeDownCast(sourceInput) &&
    this->SourceFieldType == svtkDataObject::FIELD_ASSOCIATION_ROWS)
  {
    dsa_source = svtkTable::SafeDownCast(sourceInput)->GetRowData();
    item_count_source = svtkTable::SafeDownCast(sourceInput)->GetNumberOfRows();
  }
  else
  {
    // ERROR
    svtkErrorMacro("Input type must be specified as a dataset, graph or table.");
    return 0;
  }

  svtkDataSetAttributes* dsa_target = nullptr;
  svtkDataSetAttributes* dsa_out = nullptr;
  int item_count_target = 0;
  if (svtkDataSet::SafeDownCast(targetInput) &&
    this->TargetFieldType == svtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    dsa_target = svtkDataSet::SafeDownCast(targetInput)->GetPointData();
    dsa_out = svtkDataSet::SafeDownCast(output)->GetPointData();
    item_count_target = svtkDataSet::SafeDownCast(targetInput)->GetNumberOfPoints();
  }
  else if (svtkDataSet::SafeDownCast(targetInput) &&
    this->TargetFieldType == svtkDataObject::FIELD_ASSOCIATION_CELLS)
  {
    dsa_target = svtkDataSet::SafeDownCast(targetInput)->GetCellData();
    dsa_out = svtkDataSet::SafeDownCast(output)->GetCellData();
    item_count_target = svtkDataSet::SafeDownCast(targetInput)->GetNumberOfCells();
  }
  else if (svtkGraph::SafeDownCast(targetInput) &&
    this->TargetFieldType == svtkDataObject::FIELD_ASSOCIATION_VERTICES)
  {
    dsa_target = svtkGraph::SafeDownCast(targetInput)->GetVertexData();
    dsa_out = svtkGraph::SafeDownCast(output)->GetVertexData();
    item_count_target = svtkGraph::SafeDownCast(targetInput)->GetNumberOfVertices();
  }
  else if (svtkGraph::SafeDownCast(targetInput) &&
    this->TargetFieldType == svtkDataObject::FIELD_ASSOCIATION_EDGES)
  {
    dsa_target = svtkGraph::SafeDownCast(targetInput)->GetEdgeData();
    dsa_out = svtkGraph::SafeDownCast(output)->GetEdgeData();
    item_count_target = svtkGraph::SafeDownCast(targetInput)->GetNumberOfEdges();
  }
  else if (svtkTable::SafeDownCast(targetInput) &&
    this->TargetFieldType == svtkDataObject::FIELD_ASSOCIATION_ROWS)
  {
    dsa_target = svtkTable::SafeDownCast(targetInput)->GetRowData();
    dsa_out = svtkTable::SafeDownCast(output)->GetRowData();
    item_count_target = svtkTable::SafeDownCast(targetInput)->GetNumberOfRows();
  }
  else
  {
    // ERROR
    svtkErrorMacro("Input type must be specified as a dataset, graph or table.");
    return 0;
  }

  if (this->SourceArrayName == nullptr || this->TargetArrayName == nullptr)
  {
    svtkErrorMacro("Must specify source and target array names for the transfer.");
    return 0;
  }

  svtkAbstractArray* sourceIdArray = dsa_source->GetPedigreeIds();
  svtkAbstractArray* targetIdArray = dsa_target->GetPedigreeIds();

  // Check for valid pedigree id arrays.
  if (!sourceIdArray)
  {
    svtkErrorMacro("SourceInput pedigree id array not found.");
    return 0;
  }
  if (!targetIdArray)
  {
    svtkErrorMacro("TargetInput pedigree id array not found.");
    return 0;
  }

  if (item_count_source != sourceIdArray->GetNumberOfTuples())
  {
    svtkErrorMacro(
      "The number of pedigree ids must be equal to the number of items in the source data object.");
    return 0;
  }
  if (item_count_target != targetIdArray->GetNumberOfTuples())
  {
    svtkErrorMacro(
      "The number of pedigree ids must be equal to the number of items in the target data object.");
    return 0;
  }

  // Create a map from sourceInput indices to targetInput indices
  // If we are using DirectMapping this is trivial
  // we just create an identity map
  int i;
  std::map<svtkIdType, svtkIdType> sourceIndexToTargetIndex;
  if (this->DirectMapping)
  {
    if (sourceIdArray->GetNumberOfTuples() > targetIdArray->GetNumberOfTuples())
    {
      svtkErrorMacro(
        "Cannot have more sourceInput tuples than targetInput values using direct mapping.");
      return 0;
    }
    // Create identity map.
    for (i = 0; i < sourceIdArray->GetNumberOfTuples(); i++)
    {
      sourceIndexToTargetIndex[i] = i;
    }
  }

  // Okay if we do not have direct mapping then we need
  // to do some templated madness to go from an arbitrary
  // type to a nice svtkIdType to svtkIdType mapping
  if (!this->DirectMapping)
  {
    std::map<svtkVariant, svtkIdType, svtkVariantLessThan> sourceInputIdMap;

    // Create a map from sourceInput id to sourceInput index
    for (i = 0; i < sourceIdArray->GetNumberOfTuples(); i++)
    {
      sourceInputIdMap[svtkGetVariantValue(sourceIdArray, i)] = i;
    }

    // Now create the map from sourceInput index to targetInput index
    for (i = 0; i < targetIdArray->GetNumberOfTuples(); i++)
    {
      svtkVariant id = svtkGetVariantValue(targetIdArray, i);
      if (sourceInputIdMap.count(id))
      {
        sourceIndexToTargetIndex[sourceInputIdMap[id]] = i;
      }
    }
  }

  svtkAbstractArray* sourceArray = dsa_source->GetAbstractArray(this->SourceArrayName);
  svtkAbstractArray* targetArray = svtkAbstractArray::CreateArray(sourceArray->GetDataType());
  targetArray->SetName(this->TargetArrayName);

  targetArray->SetNumberOfComponents(sourceArray->GetNumberOfComponents());
  targetArray->SetNumberOfTuples(targetIdArray->GetNumberOfTuples());

  for (i = 0; i < targetArray->GetNumberOfTuples(); i++)
  {
    targetArray->InsertVariantValue(i, this->DefaultValue);
  }

  for (i = 0; i < sourceArray->GetNumberOfTuples(); i++)
  {
    if (sourceArray->GetVariantValue(i) < 0)
    {
      cout << sourceIndexToTargetIndex[i] << " " << sourceArray->GetVariantValue(i).ToString()
           << " " << sourceArray->GetNumberOfTuples() << " " << sourceIdArray->GetNumberOfTuples()
           << " " << i << endl;

      svtkErrorMacro("Bad value...");
      continue;
    }
    targetArray->SetTuple(sourceIndexToTargetIndex[i], i, sourceArray);
  }

  dsa_out->AddArray(targetArray);
  targetArray->Delete();

  return 1;
}

svtkVariant svtkTransferAttributes::GetDefaultValue()
{
  return this->DefaultValue;
}

void svtkTransferAttributes::SetDefaultValue(svtkVariant value)
{
  this->DefaultValue = value;
}

void svtkTransferAttributes::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DirectMapping: " << this->DirectMapping << endl;
  os << indent << "DefaultValue: " << this->DefaultValue.ToString() << endl;
  os << indent << "SourceArrayName: " << (this->SourceArrayName ? this->SourceArrayName : "(none)")
     << endl;
  os << indent << "TargetArrayName: " << (this->TargetArrayName ? this->TargetArrayName : "(none)")
     << endl;
  os << indent << "SourceFieldType: " << this->SourceFieldType << endl;
  os << indent << "TargetFieldType: " << this->TargetFieldType << endl;
}
