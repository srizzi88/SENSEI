/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeFieldAggregator.cxx

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

#include "svtkTreeFieldAggregator.h"

#include "svtkAdjacentVertexIterator.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkGraph.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtkTreeDFSIterator.h"
#include "svtkVariantArray.h"

#include <cmath>

svtkStandardNewMacro(svtkTreeFieldAggregator);

svtkTreeFieldAggregator::svtkTreeFieldAggregator()
{
  this->MinValue = 0;
  this->Field = nullptr;
  this->LeafVertexUnitSize = true;
  this->LogScale = false;
}

svtkTreeFieldAggregator::~svtkTreeFieldAggregator()
{
  this->SetField(nullptr);
}

int svtkTreeFieldAggregator::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkTree* input = svtkTree::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTree* output = svtkTree::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Shallow copy the input
  output->ShallowCopy(input);

  // Check for the existence of the field to be aggregated
  if (!output->GetVertexData()->HasArray(this->Field))
  {
    this->LeafVertexUnitSize = true;
  }

  // Extract the field from the tree
  svtkAbstractArray* arr;
  if (this->LeafVertexUnitSize)
  {
    arr = svtkIntArray::New();
    arr->SetNumberOfTuples(output->GetNumberOfVertices());
    arr->SetName(this->Field);
    for (svtkIdType i = 0; i < arr->GetNumberOfTuples(); i++)
    {
      svtkArrayDownCast<svtkIntArray>(arr)->SetTuple1(i, 1);
    }
    output->GetVertexData()->AddArray(arr);
    arr->Delete();
  }
  else
  {
    svtkAbstractArray* oldArr = output->GetVertexData()->GetAbstractArray(this->Field);
    if (oldArr->GetNumberOfComponents() != 1)
    {
      svtkErrorMacro(<< "The field " << this->Field << " must have one component per tuple");
    }
    if (oldArr->IsA("svtkStringArray"))
    {
      svtkDoubleArray* doubleArr = svtkDoubleArray::New();
      doubleArr->Resize(oldArr->GetNumberOfTuples());
      for (svtkIdType i = 0; i < oldArr->GetNumberOfTuples(); i++)
      {
        doubleArr->InsertNextTuple1(svtkTreeFieldAggregator::GetDoubleValue(oldArr, i));
      }
      arr = doubleArr;
    }
    else
    {
      arr = svtkAbstractArray::CreateArray(oldArr->GetDataType());
      arr->DeepCopy(oldArr);
    }
    arr->SetName(this->Field);

    // We would like to do just perform output->GetVertexData()->RemoveArray(this->Field),
    // but because of a bug in svtkDataSetAttributes::RemoveArray(char*), we need to do it this way.
    svtkFieldData* data = svtkFieldData::SafeDownCast(output->GetVertexData());
    data->RemoveArray(this->Field);

    output->GetVertexData()->AddArray(arr);
    arr->Delete();
  }

  // Set up DFS iterator that traverses
  // children before the parent (i.e. bottom-up).
  svtkSmartPointer<svtkTreeDFSIterator> dfs = svtkSmartPointer<svtkTreeDFSIterator>::New();
  dfs->SetTree(output);
  dfs->SetMode(svtkTreeDFSIterator::FINISH);

  // Create a iterator for getting children.
  svtkSmartPointer<svtkAdjacentVertexIterator> it = svtkSmartPointer<svtkAdjacentVertexIterator>::New();

  // Iterator through the tree, aggregating child values into parent.
  while (dfs->HasNext())
  {
    svtkIdType vertex = dfs->Next();
    double value = 0;
    if (output->IsLeaf(vertex))
    {
      value = svtkTreeFieldAggregator::GetDoubleValue(arr, vertex);
      if (this->LogScale)
      {
        value = log10(value);
        if (value < this->MinValue)
        {
          value = this->MinValue;
        }
      }
    }
    else
    {
      output->GetChildren(vertex, it);
      while (it->HasNext())
      {
        value += svtkTreeFieldAggregator::GetDoubleValue(arr, it->Next());
      }
    }
    svtkTreeFieldAggregator::SetDoubleValue(arr, vertex, value);
  }

  return 1;
}

void svtkTreeFieldAggregator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Field: " << (this->Field ? this->Field : "(none)") << endl;
  os << indent << "LeafVertexUnitSize: " << (this->LeafVertexUnitSize ? "On" : "Off") << endl;
  os << indent << "MinValue: " << this->MinValue << endl;
  os << indent << "LogScale: " << (this->LogScale ? "On" : "Off") << endl;
}

double svtkTreeFieldAggregator::GetDoubleValue(svtkAbstractArray* arr, svtkIdType id)
{
  if (arr->IsA("svtkDataArray"))
  {
    double d = svtkArrayDownCast<svtkDataArray>(arr)->GetTuple1(id);
    if (d < this->MinValue)
    {
      return MinValue;
    }
    return d;
  }
  else if (arr->IsA("svtkVariantArray"))
  {
    svtkVariant v = svtkArrayDownCast<svtkVariantArray>(arr)->GetValue(id);
    if (!v.IsValid())
    {
      return this->MinValue;
    }
    bool ok;
    double d = v.ToDouble(&ok);
    if (!ok)
    {
      return this->MinValue;
    }
    if (d < this->MinValue)
    {
      return MinValue;
    }
    return d;
  }
  else if (arr->IsA("svtkStringArray"))
  {
    svtkVariant v(svtkArrayDownCast<svtkStringArray>(arr)->GetValue(id));
    bool ok;
    double d = v.ToDouble(&ok);
    if (!ok)
    {
      return this->MinValue;
    }
    if (d < this->MinValue)
    {
      return MinValue;
    }
    return d;
  }
  return this->MinValue;
}

void svtkTreeFieldAggregator::SetDoubleValue(svtkAbstractArray* arr, svtkIdType id, double value)
{
  if (arr->IsA("svtkDataArray"))
  {
    svtkArrayDownCast<svtkDataArray>(arr)->SetTuple1(id, value);
  }
  else if (arr->IsA("svtkVariantArray"))
  {
    svtkArrayDownCast<svtkVariantArray>(arr)->SetValue(id, svtkVariant(value));
  }
  else if (arr->IsA("svtkStringArray"))
  {
    svtkArrayDownCast<svtkStringArray>(arr)->SetValue(id, svtkVariant(value).ToString());
  }
}
