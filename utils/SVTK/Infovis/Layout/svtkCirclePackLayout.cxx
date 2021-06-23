/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkCirclePackLayout.cxx

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

#include "svtkCirclePackLayout.h"

#include "svtkAdjacentVertexIterator.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCirclePackLayoutStrategy.h"
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkTree.h"
#include "svtkTreeDFSIterator.h"

svtkStandardNewMacro(svtkCirclePackLayout);

svtkCirclePackLayout::svtkCirclePackLayout()
{
  this->CirclesFieldName = nullptr;
  this->LayoutStrategy = nullptr;
  this->SetCirclesFieldName("circles");
  this->SetSizeArrayName("size");
}

svtkCirclePackLayout::~svtkCirclePackLayout()
{
  this->SetCirclesFieldName(nullptr);
  if (this->LayoutStrategy)
  {
    this->LayoutStrategy->Delete();
  }
}

svtkCxxSetObjectMacro(svtkCirclePackLayout, LayoutStrategy, svtkCirclePackLayoutStrategy);

void svtkCirclePackLayout::prepareSizeArray(svtkDoubleArray* mySizeArray, svtkTree* tree)
{
  svtkTreeDFSIterator* dfs = svtkTreeDFSIterator::New();
  dfs->SetMode(svtkTreeDFSIterator::FINISH);
  dfs->SetTree(tree);

  double currentLeafSize = 0.0;
  while (dfs->HasNext())
  {
    svtkIdType vertex = dfs->Next();

    if (tree->IsLeaf(vertex))
    {
      double size = mySizeArray->GetValue(vertex);
      if (size == 0.0)
      {
        size = 1.0;
        mySizeArray->SetValue(vertex, size);
      }
      currentLeafSize += size;
    }
    else
    {
      mySizeArray->SetValue(vertex, currentLeafSize);
    }
  }

  dfs->Delete();
}

int svtkCirclePackLayout::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->LayoutStrategy == nullptr)
  {
    svtkErrorMacro(<< "Layout strategy must be non-null.");
    return 0;
  }
  if (this->CirclesFieldName == nullptr)
  {
    svtkErrorMacro(<< "Circles field name must be non-null.");
    return 0;
  }
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Storing the inputTree and outputTree handles
  svtkTree* inputTree = svtkTree::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTree* outputTree = svtkTree::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Check for size array on the input svtkTree.
  svtkDataArray* sizeArray = this->GetInputArrayToProcess(0, inputTree);
  svtkDoubleArray* mySizeArray = svtkDoubleArray::New();
  if (sizeArray)
  {
    mySizeArray->DeepCopy(sizeArray);
  }
  else
  {
    mySizeArray->FillComponent(0, 0.0);
    mySizeArray->SetNumberOfTuples(inputTree->GetNumberOfVertices());
  }

  this->prepareSizeArray(mySizeArray, inputTree);

  // Copy the input into the output
  outputTree->ShallowCopy(inputTree);

  // Add the 3-tuple array that will store the Xcenter, Ycenter, and Radius
  svtkDoubleArray* coordsArray = svtkDoubleArray::New();
  coordsArray->SetName(this->CirclesFieldName);
  coordsArray->SetNumberOfComponents(3);
  coordsArray->SetNumberOfTuples(inputTree->GetNumberOfVertices());
  svtkDataSetAttributes* data = outputTree->GetVertexData();
  data->AddArray(coordsArray);
  coordsArray->Delete();

  // Find circle packing layout
  this->LayoutStrategy->Layout(inputTree, coordsArray, mySizeArray);

  mySizeArray->Delete();

  // Copy the coordinates from the layout into the Points field
  svtkPoints* points = outputTree->GetPoints();
  points->SetNumberOfPoints(coordsArray->GetNumberOfTuples());
  for (int i = 0; i < coordsArray->GetNumberOfTuples(); ++i)
  {
    double where[3];
    coordsArray->GetTuple(i, where);
    where[2] = 0;
    points->SetPoint(i, where);
  }
  return 1;
}

void svtkCirclePackLayout::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "CirclesFieldName: " << (this->CirclesFieldName ? this->CirclesFieldName : "(none)")
     << endl;
  os << indent << "LayoutStrategy: " << (this->LayoutStrategy ? "" : "(none)") << endl;
  if (this->LayoutStrategy)
  {
    this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
  }
}

svtkIdType svtkCirclePackLayout::FindVertex(double pnt[2], double* cinfo)
{
  // Do we have an output?
  svtkTree* otree = this->GetOutput();
  if (!otree)
  {
    svtkErrorMacro(<< "Could not get output tree.");
    return -1;
  }

  // Get the three tuple array for the points
  svtkDataArray* array = otree->GetVertexData()->GetArray(this->CirclesFieldName);
  if (!array)
  {
    svtkErrorMacro(<< "Output Tree does not contain circle packing information.");
    return -1;
  }

  // Check to see that we are in the dataset at all
  double climits[3];

  svtkIdType vertex = otree->GetRoot();
  svtkDoubleArray* circleInfo = svtkArrayDownCast<svtkDoubleArray>(array);
  // Now try to find the vertex that contains the point
  circleInfo->GetTypedTuple(vertex, climits); // Get the extents of the root
  if (pow((pnt[0] - climits[0]), 2) + pow((pnt[1] - climits[1]), 2) > pow(climits[2], 2))
  {
    // Point is not in the tree at all
    return -1;
  }

  // Now traverse the children to try and find
  // the vertex that contains the point
  svtkIdType child;
  if (cinfo)
  {
    cinfo[0] = climits[0];
    cinfo[1] = climits[1];
    cinfo[2] = climits[2];
  }

  svtkAdjacentVertexIterator* it = svtkAdjacentVertexIterator::New();
  otree->GetAdjacentVertices(vertex, it);
  while (it->HasNext())
  {
    child = it->Next();
    circleInfo->GetTypedTuple(child, climits); // Get the extents of the child
    if (pow((pnt[0] - climits[0]), 2) + pow((pnt[1] - climits[1]), 2) > pow(climits[2], 2))
    {
      continue;
    }
    // If we are here then the point is contained by the child
    // So recurse down the children of this vertex
    vertex = child;
    if (cinfo)
    {
      cinfo[0] = climits[0];
      cinfo[1] = climits[1];
      cinfo[2] = climits[2];
    }
    otree->GetAdjacentVertices(vertex, it);
  }
  it->Delete();

  return vertex;
}

void svtkCirclePackLayout::GetBoundingCircle(svtkIdType id, double* cinfo)
{
  // Do we have an output?
  svtkTree* otree = this->GetOutput();
  if (!otree)
  {
    svtkErrorMacro(<< "Could not get output tree.");
    return;
  }

  if (!cinfo)
  {
    svtkErrorMacro(<< "cinfo is nullptr");
    return;
  }

  // Get the three tuple array for the circle
  svtkDataArray* array = otree->GetVertexData()->GetArray(this->CirclesFieldName);
  if (!array)
  {
    svtkErrorMacro(<< "Output Tree does not contain circle packing information.");
    return;
  }

  svtkDoubleArray* boxInfo = svtkArrayDownCast<svtkDoubleArray>(array);
  boxInfo->GetTypedTuple(id, cinfo);
}

svtkMTimeType svtkCirclePackLayout::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->LayoutStrategy != nullptr)
  {
    time = this->LayoutStrategy->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}
