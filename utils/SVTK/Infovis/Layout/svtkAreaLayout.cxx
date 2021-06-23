/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAreaLayout.cxx

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

#include "svtkAreaLayout.h"

#include "svtkAdjacentVertexIterator.h"
#include "svtkAreaLayoutStrategy.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkTree.h"
#include "svtkTreeDFSIterator.h"
#include "svtkTreeFieldAggregator.h"

svtkStandardNewMacro(svtkAreaLayout);
svtkCxxSetObjectMacro(svtkAreaLayout, LayoutStrategy, svtkAreaLayoutStrategy);

svtkAreaLayout::svtkAreaLayout()
{
  this->AreaArrayName = nullptr;
  this->LayoutStrategy = nullptr;
  this->SetAreaArrayName("area");
  this->EdgeRoutingPoints = true;
  this->SetSizeArrayName("size");
  this->SetNumberOfOutputPorts(2);
}

svtkAreaLayout::~svtkAreaLayout()
{
  this->SetAreaArrayName(nullptr);
  if (this->LayoutStrategy)
  {
    this->LayoutStrategy->Delete();
  }
}

int svtkAreaLayout::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->LayoutStrategy == nullptr)
  {
    svtkErrorMacro(<< "Layout strategy must be non-null.");
    return 0;
  }
  if (this->AreaArrayName == nullptr)
  {
    svtkErrorMacro(<< "Sector array name must be non-null.");
    return 0;
  }
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* outEdgeRoutingInfo = outputVector->GetInformationObject(1);

  // Storing the inputTree and outputTree handles
  svtkTree* inputTree = svtkTree::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTree* outputTree = svtkTree::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkTree* outputEdgeRoutingTree =
    svtkTree::SafeDownCast(outEdgeRoutingInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Copy the input into the output
  outputTree->ShallowCopy(inputTree);
  outputEdgeRoutingTree->ShallowCopy(inputTree);

  // Add the 4-tuple array that will store the min,max xy coords
  svtkFloatArray* coordsArray = svtkFloatArray::New();
  coordsArray->SetName(this->AreaArrayName);
  coordsArray->SetNumberOfComponents(4);
  coordsArray->SetNumberOfTuples(outputTree->GetNumberOfVertices());
  svtkDataSetAttributes* data = outputTree->GetVertexData();
  data->AddArray(coordsArray);
  coordsArray->Delete();

  if (!this->EdgeRoutingPoints)
  {
    outputEdgeRoutingTree = nullptr;
  }

  svtkSmartPointer<svtkDataArray> sizeArray = this->GetInputArrayToProcess(0, inputTree);
  if (!sizeArray)
  {
    svtkSmartPointer<svtkTreeFieldAggregator> agg = svtkSmartPointer<svtkTreeFieldAggregator>::New();
    svtkSmartPointer<svtkTree> t = svtkSmartPointer<svtkTree>::New();
    t->ShallowCopy(outputTree);
    agg->SetInputData(t);
    agg->SetField("size");
    agg->SetLeafVertexUnitSize(true);
    agg->Update();
    sizeArray = agg->GetOutput()->GetVertexData()->GetArray("size");
  }

  // Okay now layout the tree :)
  this->LayoutStrategy->Layout(outputTree, coordsArray, sizeArray);
  this->LayoutStrategy->LayoutEdgePoints(outputTree, coordsArray, sizeArray, outputEdgeRoutingTree);

  return 1;
}

void svtkAreaLayout::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AreaArrayName: " << (this->AreaArrayName ? this->AreaArrayName : "(none)")
     << endl;
  os << indent << "EdgeRoutingPoints: " << this->EdgeRoutingPoints << endl;
  os << indent << "LayoutStrategy: " << (this->LayoutStrategy ? "" : "(none)") << endl;
  if (this->LayoutStrategy)
  {
    this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
  }
}

svtkIdType svtkAreaLayout::FindVertex(float pnt[2])
{
  // Do we have an output?
  svtkTree* otree = this->GetOutput();
  if (!otree)
  {
    svtkErrorMacro(<< "Could not get output tree.");
    return -1;
  }

  // Get the four tuple array for the points
  svtkDataArray* array = otree->GetVertexData()->GetArray(this->AreaArrayName);
  if (!array)
  {
    return -1;
  }

  if (otree->GetNumberOfVertices() == 0)
  {
    return -1;
  }

  return this->LayoutStrategy->FindVertex(otree, array, pnt);
}

void svtkAreaLayout::GetBoundingArea(svtkIdType id, float* sinfo)
{
  // Do we have an output?
  svtkTree* otree = this->GetOutput();
  if (!otree)
  {
    svtkErrorMacro(<< "Could not get output tree.");
    return;
  }

  // Get the four tuple array for the points
  svtkDataArray* array = otree->GetVertexData()->GetArray(this->AreaArrayName);
  if (!array)
  {
    return;
  }

  svtkFloatArray* sectorInfo = svtkArrayDownCast<svtkFloatArray>(array);
  sectorInfo->GetTypedTuple(id, sinfo);
}

svtkMTimeType svtkAreaLayout::GetMTime()
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
