/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeMapLayout.cxx

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

#include "svtkTreeMapLayout.h"

#include "svtkAdjacentVertexIterator.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkTree.h"
#include "svtkTreeMapLayoutStrategy.h"

svtkStandardNewMacro(svtkTreeMapLayout);

svtkTreeMapLayout::svtkTreeMapLayout()
{
  this->RectanglesFieldName = nullptr;
  this->LayoutStrategy = nullptr;
  this->SetRectanglesFieldName("area");
  this->SetSizeArrayName("size");
}

svtkTreeMapLayout::~svtkTreeMapLayout()
{
  this->SetRectanglesFieldName(nullptr);
  if (this->LayoutStrategy)
  {
    this->LayoutStrategy->Delete();
  }
}

svtkCxxSetObjectMacro(svtkTreeMapLayout, LayoutStrategy, svtkTreeMapLayoutStrategy);

int svtkTreeMapLayout::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->LayoutStrategy == nullptr)
  {
    svtkErrorMacro(<< "Layout strategy must be non-null.");
    return 0;
  }
  if (this->RectanglesFieldName == nullptr)
  {
    svtkErrorMacro(<< "Rectangles field name must be non-null.");
    return 0;
  }
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Storing the inputTree and outputTree handles
  svtkTree* inputTree = svtkTree::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTree* outputTree = svtkTree::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Copy the input into the output
  outputTree->ShallowCopy(inputTree);

  // Add the 4-tuple array that will store the min,max xy coords
  svtkFloatArray* coordsArray = svtkFloatArray::New();
  coordsArray->SetName(this->RectanglesFieldName);
  coordsArray->SetNumberOfComponents(4);
  coordsArray->SetNumberOfTuples(inputTree->GetNumberOfVertices());
  svtkDataSetAttributes* data = outputTree->GetVertexData();
  data->AddArray(coordsArray);
  coordsArray->Delete();

  // Add the 4-tuple array that will store the min,max xy coords
  svtkDataArray* sizeArray = this->GetInputArrayToProcess(0, inputTree);
  if (!sizeArray)
  {
    svtkErrorMacro("Size array not found.");
    return 0;
  }

  // Okay now layout the tree :)
  this->LayoutStrategy->Layout(inputTree, coordsArray, sizeArray);

  return 1;
}

void svtkTreeMapLayout::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RectanglesFieldName: "
     << (this->RectanglesFieldName ? this->RectanglesFieldName : "(none)") << endl;
  os << indent << "LayoutStrategy: " << (this->LayoutStrategy ? "" : "(none)") << endl;
  if (this->LayoutStrategy)
  {
    this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
  }
}

svtkIdType svtkTreeMapLayout::FindVertex(float pnt[2], float* binfo)
{
  // Do we have an output?
  svtkTree* otree = this->GetOutput();
  if (!otree)
  {
    svtkErrorMacro(<< "Could not get output tree.");
    return -1;
  }

  // Get the four tuple array for the points
  svtkDataArray* array = otree->GetVertexData()->GetArray(this->RectanglesFieldName);
  if (!array)
  {
    // svtkErrorMacro(<< "Output Tree does not have box information.");
    return -1;
  }

  // Check to see that we are in the dataset at all
  float blimits[4];

  svtkIdType vertex = otree->GetRoot();
  svtkFloatArray* boxInfo = svtkArrayDownCast<svtkFloatArray>(array);
  // Now try to find the vertex that contains the point
  boxInfo->GetTypedTuple(vertex, blimits); // Get the extents of the root
  if ((pnt[0] < blimits[0]) || (pnt[0] > blimits[1]) || (pnt[1] < blimits[2]) ||
    (pnt[1] > blimits[3]))
  {
    // Point is not in the tree at all
    return -1;
  }

  // Now traverse the children to try and find
  // the vertex that contains the point
  svtkIdType child;
  if (binfo)
  {
    binfo[0] = blimits[0];
    binfo[1] = blimits[1];
    binfo[2] = blimits[2];
    binfo[3] = blimits[3];
  }

  svtkAdjacentVertexIterator* it = svtkAdjacentVertexIterator::New();
  otree->GetAdjacentVertices(vertex, it);
  while (it->HasNext())
  {
    child = it->Next();
    boxInfo->GetTypedTuple(child, blimits); // Get the extents of the child
    if ((pnt[0] < blimits[0]) || (pnt[0] > blimits[1]) || (pnt[1] < blimits[2]) ||
      (pnt[1] > blimits[3]))
    {
      continue;
    }
    // If we are here then the point is contained by the child
    // So recurse down the children of this vertex
    vertex = child;
    otree->GetAdjacentVertices(vertex, it);
  }
  it->Delete();

  return vertex;
}

void svtkTreeMapLayout::GetBoundingBox(svtkIdType id, float* binfo)
{
  // Do we have an output?
  svtkTree* otree = this->GetOutput();
  if (!otree)
  {
    svtkErrorMacro(<< "Could not get output tree.");
    return;
  }

  // Get the four tuple array for the points
  svtkDataArray* array = otree->GetVertexData()->GetArray(this->RectanglesFieldName);
  if (!array)
  {
    // svtkErrorMacro(<< "Output Tree does not have box information.");
    return;
  }

  svtkFloatArray* boxInfo = svtkArrayDownCast<svtkFloatArray>(array);
  boxInfo->GetTypedTuple(id, binfo);
}

svtkMTimeType svtkTreeMapLayout::GetMTime()
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
