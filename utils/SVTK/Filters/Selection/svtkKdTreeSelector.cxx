/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkKdTreeSelector.cxx

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

#include "svtkKdTreeSelector.h"

#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkKdTree.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkKdTreeSelector);

svtkKdTreeSelector::svtkKdTreeSelector()
{
  this->SelectionBounds[0] = 0.0;
  this->SelectionBounds[1] = -1.0;
  this->SelectionBounds[2] = 0.0;
  this->SelectionBounds[3] = -1.0;
  this->SelectionBounds[4] = SVTK_DOUBLE_MIN;
  this->SelectionBounds[5] = SVTK_DOUBLE_MAX;
  this->KdTree = nullptr;
  this->BuildKdTreeFromInput = true;
  this->SelectionFieldName = nullptr;
  this->SingleSelection = false;
  this->SingleSelectionThreshold = 1.0;
  this->SelectionAttribute = -1;
}

svtkKdTreeSelector::~svtkKdTreeSelector()
{
  this->SetKdTree(nullptr);
  this->SetSelectionFieldName(nullptr);
}

void svtkKdTreeSelector::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "KdTree: " << (this->KdTree ? "" : "(null)") << endl;
  if (this->KdTree)
  {
    this->KdTree->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent
     << "SelectionFieldName: " << (this->SelectionFieldName ? this->SelectionFieldName : "(null)")
     << endl;
  os << indent << "BuildKdTreeFromInput: " << (this->BuildKdTreeFromInput ? "on" : "off") << endl;
  os << indent << "SelectionBounds: " << endl;
  os << indent << "  xmin, xmax = (" << this->SelectionBounds[0] << "," << this->SelectionBounds[1]
     << ")" << endl;
  os << indent << "  ymin, ymax = (" << this->SelectionBounds[2] << "," << this->SelectionBounds[3]
     << ")" << endl;
  os << indent << "  zmin, zmax = (" << this->SelectionBounds[4] << "," << this->SelectionBounds[5]
     << ")" << endl;
  os << indent << "SingleSelection: " << (this->SingleSelection ? "on" : "off") << endl;
  os << indent << "SingleSelectionThreshold: " << this->SingleSelectionThreshold << endl;
  os << indent << "SelectionAttribute: " << this->SelectionAttribute << endl;
}

void svtkKdTreeSelector::SetKdTree(svtkKdTree* arg)
{
  svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting KdTree to " << arg);
  if (this->KdTree != arg)
  {
    svtkKdTree* tempSGMacroVar = this->KdTree;
    this->KdTree = arg;
    if (this->KdTree != nullptr)
    {
      this->BuildKdTreeFromInput = false;
      this->KdTree->Register(this);
    }
    else
    {
      this->BuildKdTreeFromInput = true;
    }
    if (tempSGMacroVar != nullptr)
    {
      tempSGMacroVar->UnRegister(this);
    }
    this->Modified();
  }
}

svtkMTimeType svtkKdTreeSelector::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  if (this->KdTree != nullptr)
  {
    svtkMTimeType time = this->KdTree->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

int svtkKdTreeSelector::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkAbstractArray* field = nullptr;
  svtkGraph* graph = nullptr;

  if (this->BuildKdTreeFromInput)
  {
    svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (inInfo == nullptr)
    {
      svtkErrorMacro("No input, but building kd-tree from input");
      return 0;
    }
    svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
    if (input == nullptr)
    {
      svtkErrorMacro("Input is nullptr");
      return 0;
    }
    graph = svtkGraph::SafeDownCast(input);
    svtkPointSet* pointSet = svtkPointSet::SafeDownCast(input);
    if (!graph && !pointSet)
    {
      svtkErrorMacro("Input must be a graph or point set");
      return 0;
    }

    svtkPoints* points = nullptr;
    if (graph)
    {
      points = graph->GetPoints();
    }
    else
    {
      points = pointSet->GetPoints();
    }

    // If no points, there is nothing to do
    if (points == nullptr || points->GetNumberOfPoints() == 0)
    {
      return 1;
    }

    // Construct the kd-tree if we need to
    if (this->KdTree == nullptr || this->KdTree->GetMTime() < input->GetMTime())
    {
      if (this->KdTree == nullptr)
      {
        this->KdTree = svtkKdTree::New();
      }
      this->KdTree->Initialize();
      this->KdTree->BuildLocatorFromPoints(points);
    }

    // Look for selection field
    if (this->SelectionAttribute == svtkDataSetAttributes::GLOBALIDS ||
      this->SelectionAttribute == svtkDataSetAttributes::PEDIGREEIDS)
    {
      if (graph)
      {
        field = graph->GetVertexData()->GetAbstractAttribute(this->SelectionAttribute);
      }
      else
      {
        field = pointSet->GetPointData()->GetAbstractAttribute(this->SelectionAttribute);
      }
      if (field == nullptr)
      {
        svtkErrorMacro("Could not find attribute " << this->SelectionAttribute);
        return 0;
      }
    }
    if (this->SelectionFieldName)
    {
      if (graph)
      {
        field = graph->GetVertexData()->GetAbstractArray(this->SelectionFieldName);
      }
      else
      {
        field = pointSet->GetPointData()->GetAbstractArray(this->SelectionFieldName);
      }
      if (field == nullptr)
      {
        svtkErrorMacro("SelectionFieldName field not found");
        return 0;
      }
    }
  }

  // If no kd-tree, there is nothing to do
  if (this->KdTree == nullptr)
  {
    return 1;
  }

  // Use the kd-tree to find the selected points
  svtkIdTypeArray* ids = svtkIdTypeArray::New();
  if (this->SingleSelection)
  {
    double center[3];
    for (int c = 0; c < 3; c++)
    {
      center[c] = (this->SelectionBounds[2 * c] + this->SelectionBounds[2 * c + 1]) / 2.0;
    }
    double dist;
    svtkIdType closestToCenter = this->KdTree->FindClosestPoint(center, dist);
    if (dist < this->SingleSelectionThreshold)
    {
      ids->InsertNextValue(closestToCenter);
    }
  }
  else
  {
    this->KdTree->FindPointsInArea(this->SelectionBounds, ids);
  }

  // Fill the selection with the found ids
  svtkSelection* output = svtkSelection::GetData(outputVector);
  svtkSmartPointer<svtkSelectionNode> node = svtkSmartPointer<svtkSelectionNode>::New();
  output->AddNode(node);
  if (graph)
  {
    node->SetFieldType(svtkSelectionNode::VERTEX);
  }
  else
  {
    node->SetFieldType(svtkSelectionNode::POINT);
  }
  if (field)
  {
    svtkAbstractArray* arr = svtkAbstractArray::CreateArray(field->GetDataType());
    arr->SetName(field->GetName());
    for (svtkIdType i = 0; i < ids->GetNumberOfTuples(); i++)
    {
      arr->InsertNextTuple(ids->GetValue(i), field);
    }
    if (this->SelectionAttribute == svtkDataSetAttributes::GLOBALIDS ||
      this->SelectionAttribute == svtkDataSetAttributes::PEDIGREEIDS)
    {
      if (this->SelectionAttribute == svtkDataSetAttributes::GLOBALIDS)
      {
        node->SetContentType(svtkSelectionNode::GLOBALIDS);
      }
      else if (this->SelectionAttribute == svtkDataSetAttributes::PEDIGREEIDS)
      {
        node->SetContentType(svtkSelectionNode::PEDIGREEIDS);
      }
    }
    else
    {
      node->SetContentType(svtkSelectionNode::VALUES);
    }
    node->SetSelectionList(arr);
    arr->Delete();
  }
  else
  {
    node->SetContentType(svtkSelectionNode::INDICES);
    node->SetSelectionList(ids);
  }

  // Clean up
  ids->Delete();

  return 1;
}

int svtkKdTreeSelector::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // Input (if specified) may be a point set or graph.
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}
