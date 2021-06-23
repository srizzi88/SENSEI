/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointSetToLabelHierarchy.cxx

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

#include "svtkPointSetToLabelHierarchy.h"

#include "svtkDataObjectTypes.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkGraph.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkLabelHierarchy.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTextProperty.h"
#include "svtkTimerLog.h"
#include "svtkUnicodeString.h"
#include "svtkUnicodeStringArray.h"

#include <vector>

svtkStandardNewMacro(svtkPointSetToLabelHierarchy);
svtkCxxSetObjectMacro(svtkPointSetToLabelHierarchy, TextProperty, svtkTextProperty);

svtkPointSetToLabelHierarchy::svtkPointSetToLabelHierarchy()
{
  this->MaximumDepth = 5;
  this->TargetLabelCount = 32;
  this->UseUnicodeStrings = false;
  this->TextProperty = svtkTextProperty::New();
  this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::POINT, "Priority");
  this->SetInputArrayToProcess(1, 0, 0, svtkDataObject::POINT, "LabelSize");
  this->SetInputArrayToProcess(2, 0, 0, svtkDataObject::POINT, "LabelText");
  this->SetInputArrayToProcess(3, 0, 0, svtkDataObject::POINT, "IconIndex");
  this->SetInputArrayToProcess(4, 0, 0, svtkDataObject::POINT, "Orientation");
  this->SetInputArrayToProcess(5, 0, 0, svtkDataObject::POINT, "BoundedSize");
}

svtkPointSetToLabelHierarchy::~svtkPointSetToLabelHierarchy()
{
  if (this->TextProperty)
  {
    this->TextProperty->Delete();
  }
}

void svtkPointSetToLabelHierarchy::SetPriorityArrayName(const char* name)
{
  this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::POINT, name);
}

const char* svtkPointSetToLabelHierarchy::GetPriorityArrayName()
{
  svtkInformation* info =
    this->GetInformation()->Get(svtkAlgorithm::INPUT_ARRAYS_TO_PROCESS())->GetInformationObject(0);
  return info->Get(svtkDataObject::FIELD_NAME());
}

void svtkPointSetToLabelHierarchy::SetSizeArrayName(const char* name)
{
  this->SetInputArrayToProcess(1, 0, 0, svtkDataObject::POINT, name);
}

const char* svtkPointSetToLabelHierarchy::GetSizeArrayName()
{
  svtkInformation* info =
    this->GetInformation()->Get(svtkAlgorithm::INPUT_ARRAYS_TO_PROCESS())->GetInformationObject(1);
  return info->Get(svtkDataObject::FIELD_NAME());
}

void svtkPointSetToLabelHierarchy::SetLabelArrayName(const char* name)
{
  this->SetInputArrayToProcess(2, 0, 0, svtkDataObject::POINT, name);
}

const char* svtkPointSetToLabelHierarchy::GetLabelArrayName()
{
  svtkInformation* info =
    this->GetInformation()->Get(svtkAlgorithm::INPUT_ARRAYS_TO_PROCESS())->GetInformationObject(2);
  return info->Get(svtkDataObject::FIELD_NAME());
}

void svtkPointSetToLabelHierarchy::SetIconIndexArrayName(const char* name)
{
  this->SetInputArrayToProcess(3, 0, 0, svtkDataObject::POINT, name);
}

const char* svtkPointSetToLabelHierarchy::GetIconIndexArrayName()
{
  svtkInformation* info =
    this->GetInformation()->Get(svtkAlgorithm::INPUT_ARRAYS_TO_PROCESS())->GetInformationObject(3);
  return info->Get(svtkDataObject::FIELD_NAME());
}

void svtkPointSetToLabelHierarchy::SetOrientationArrayName(const char* name)
{
  this->SetInputArrayToProcess(4, 0, 0, svtkDataObject::POINT, name);
}

const char* svtkPointSetToLabelHierarchy::GetOrientationArrayName()
{
  svtkInformation* info =
    this->GetInformation()->Get(svtkAlgorithm::INPUT_ARRAYS_TO_PROCESS())->GetInformationObject(4);
  return info->Get(svtkDataObject::FIELD_NAME());
}

void svtkPointSetToLabelHierarchy::SetBoundedSizeArrayName(const char* name)
{
  this->SetInputArrayToProcess(5, 0, 0, svtkDataObject::POINT, name);
}

const char* svtkPointSetToLabelHierarchy::GetBoundedSizeArrayName()
{
  svtkInformation* info =
    this->GetInformation()->Get(svtkAlgorithm::INPUT_ARRAYS_TO_PROCESS())->GetInformationObject(5);
  return info->Get(svtkDataObject::FIELD_NAME());
}

int svtkPointSetToLabelHierarchy::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
    info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  }
  return 1;
}

int svtkPointSetToLabelHierarchy::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  svtkSmartPointer<svtkTimerLog> timer = svtkSmartPointer<svtkTimerLog>::New();
  timer->StartTimer();

  svtkIdType numPoints = 0;
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataObject* inData = inInfo->Get(svtkDataObject::DATA_OBJECT());

  svtkGraph* graph = svtkGraph::SafeDownCast(inData);
  if (graph)
  {
    numPoints = graph->GetNumberOfVertices();
  }

  svtkPointSet* ptset = svtkPointSet::SafeDownCast(inData);
  if (ptset)
  {
    numPoints = ptset->GetNumberOfPoints();
  }

  int maxDepth = this->MaximumDepth;
  // maxDepth = (int)ceil(log(1.0 + 7.0*totalPoints/this->TargetLabelCount) / log(8.0));

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkLabelHierarchy* ouData =
    svtkLabelHierarchy::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!ouData)
  {
    svtkErrorMacro("No output data");
    return 0;
  }

  ouData->SetTargetLabelCount(this->TargetLabelCount);
  ouData->SetMaximumDepth(maxDepth);

  if (!inData)
  {
    svtkErrorMacro("Null input data");
    return 0;
  }

  svtkPoints* pts = nullptr;
  svtkDataSetAttributes* pdata = nullptr;

  if (graph)
  {
    pts = graph->GetPoints();
    pdata = graph->GetVertexData();
  }

  if (ptset)
  {
    pts = ptset->GetPoints();
    pdata = ptset->GetPointData();
  }

  svtkDataArray* priorities =
    svtkArrayDownCast<svtkDataArray>(this->GetInputAbstractArrayToProcess(0, inputVector));
  svtkDataArray* sizes =
    svtkArrayDownCast<svtkDataArray>(this->GetInputAbstractArrayToProcess(1, inputVector));
  svtkAbstractArray* labels = this->GetInputAbstractArrayToProcess(2, inputVector);
  svtkIntArray* iconIndices =
    svtkArrayDownCast<svtkIntArray>(this->GetInputAbstractArrayToProcess(3, inputVector));
  svtkDataArray* orientations =
    svtkArrayDownCast<svtkDataArray>(this->GetInputAbstractArrayToProcess(4, inputVector));
  svtkDataArray* boundedSizes =
    svtkArrayDownCast<svtkDataArray>(this->GetInputAbstractArrayToProcess(5, inputVector));

  if (!ouData->GetPoints())
  {
    svtkPoints* oupts = svtkPoints::New();
    ouData->SetPoints(oupts);
    oupts->FastDelete();
  }
  if (pts)
  {
    ouData->GetPoints()->ShallowCopy(pts);
  }
  ouData->GetPointData()->ShallowCopy(pdata);
  svtkSmartPointer<svtkIntArray> type = svtkSmartPointer<svtkIntArray>::New();
  type->SetName("Type");
  type->SetNumberOfTuples(numPoints);
  type->FillComponent(0, 0);
  ouData->GetPointData()->AddArray(type);
  ouData->SetPriorities(priorities);
  if (labels)
  {
    if ((this->UseUnicodeStrings && svtkArrayDownCast<svtkUnicodeStringArray>(labels)) ||
      (!this->UseUnicodeStrings && svtkArrayDownCast<svtkStringArray>(labels)))
    {
      ouData->SetLabels(labels);
    }
    else if (this->UseUnicodeStrings)
    {
      svtkSmartPointer<svtkUnicodeStringArray> arr = svtkSmartPointer<svtkUnicodeStringArray>::New();
      svtkIdType numComps = labels->GetNumberOfComponents();
      svtkIdType numTuples = labels->GetNumberOfTuples();
      arr->SetNumberOfComponents(numComps);
      arr->SetNumberOfTuples(numTuples);
      for (svtkIdType i = 0; i < numTuples; ++i)
      {
        for (svtkIdType j = 0; j < numComps; ++j)
        {
          svtkIdType ind = i * numComps + j;
          arr->SetValue(ind, labels->GetVariantValue(ind).ToUnicodeString());
        }
      }
      arr->SetName(labels->GetName());
      ouData->GetPointData()->AddArray(arr);
      ouData->SetLabels(arr);
    }
    else
    {
      svtkSmartPointer<svtkStringArray> arr = svtkSmartPointer<svtkStringArray>::New();
      svtkIdType numComps = labels->GetNumberOfComponents();
      svtkIdType numTuples = labels->GetNumberOfTuples();
      arr->SetNumberOfComponents(numComps);
      arr->SetNumberOfTuples(numTuples);
      for (svtkIdType i = 0; i < numTuples; ++i)
      {
        for (svtkIdType j = 0; j < numComps; ++j)
        {
          svtkIdType ind = i * numComps + j;
          arr->SetValue(ind, labels->GetVariantValue(ind).ToString());
        }
      }
      arr->SetName(labels->GetName());
      ouData->GetPointData()->AddArray(arr);
      ouData->SetLabels(arr);
    }
  }
  ouData->SetIconIndices(iconIndices);
  ouData->SetOrientations(orientations);
  ouData->SetSizes(sizes);
  ouData->SetBoundedSizes(boundedSizes);
  ouData->SetTextProperty(this->TextProperty);
  ouData->ComputeHierarchy();

  timer->StopTimer();
  svtkDebugMacro("StartupTime: " << timer->GetElapsedTime() << endl);

  return 1;
}

void svtkPointSetToLabelHierarchy::PrintSelf(ostream& os, svtkIndent indent)
{
  os << indent << "MaximumDepth: " << this->MaximumDepth << "\n";
  os << indent << "TargetLabelCount: " << this->TargetLabelCount << "\n";
  os << indent << "UseUnicodeStrings: " << this->UseUnicodeStrings << "\n";
  os << indent << "TextProperty: " << this->TextProperty << "\n";
  this->Superclass::PrintSelf(os, indent);
}
