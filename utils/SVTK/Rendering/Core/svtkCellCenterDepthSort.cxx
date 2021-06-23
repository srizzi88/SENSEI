/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellCenterDepthSort.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */
#include "svtkCellCenterDepthSort.h"

#include "svtkCamera.h"
#include "svtkCell.h"
#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkSortDataArray.h"

#include <algorithm>
#include <stack>
#include <utility>

//-----------------------------------------------------------------------------

typedef std::pair<svtkIdType, svtkIdType> svtkIdPair;

class svtkCellCenterDepthSortStack
{
public:
  std::stack<svtkIdPair> Stack;
};

//-----------------------------------------------------------------------------

svtkStandardNewMacro(svtkCellCenterDepthSort);

svtkCellCenterDepthSort::svtkCellCenterDepthSort()
{
  this->SortedCells = svtkIdTypeArray::New();
  this->SortedCells->SetNumberOfComponents(1);
  this->SortedCellPartition = svtkIdTypeArray::New();
  this->SortedCells->SetNumberOfComponents(1);

  this->CellCenters = svtkFloatArray::New();
  this->CellCenters->SetNumberOfComponents(3);
  this->CellDepths = svtkFloatArray::New();
  this->CellDepths->SetNumberOfComponents(1);
  this->CellPartitionDepths = svtkFloatArray::New();
  this->CellPartitionDepths->SetNumberOfComponents(1);

  this->ToSort = new svtkCellCenterDepthSortStack;
}

svtkCellCenterDepthSort::~svtkCellCenterDepthSort()
{
  this->SortedCells->Delete();
  this->SortedCellPartition->Delete();
  this->CellCenters->Delete();
  this->CellDepths->Delete();
  this->CellPartitionDepths->Delete();

  delete this->ToSort;
}

void svtkCellCenterDepthSort::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

float* svtkCellCenterDepthSort::ComputeProjectionVector()
{
  svtkDebugMacro("ComputeProjectionVector");

  if (this->Camera == nullptr)
  {
    svtkErrorMacro("Must set camera before sorting cells.");
    static float v[3] = { 0.0, 0.0, 0.0 };
    return v;
  }

  double focalPoint[4];
  double position[4];

  this->Camera->GetFocalPoint(focalPoint);
  focalPoint[3] = 1.0;
  this->Camera->GetPosition(position);
  position[3] = 1.0;

  this->InverseModelTransform->MultiplyPoint(focalPoint, focalPoint);
  this->InverseModelTransform->MultiplyPoint(position, position);

  static float vector[3];
  if (this->Direction == svtkVisibilitySort::BACK_TO_FRONT)
  {
    // Sort back to front.
    vector[0] = position[0] - focalPoint[0];
    vector[1] = position[1] - focalPoint[1];
    vector[2] = position[2] - focalPoint[2];
  }
  else
  {
    // Sort front to back.
    vector[0] = focalPoint[0] - position[0];
    vector[1] = focalPoint[1] - position[1];
    vector[2] = focalPoint[2] - position[2];
  }

  svtkDebugMacro("Returning: " << vector[0] << ", " << vector[1] << ", " << vector[2]);

  return vector;
}

void svtkCellCenterDepthSort::ComputeCellCenters()
{
  svtkIdType numcells = this->Input->GetNumberOfCells();
  this->CellCenters->SetNumberOfTuples(numcells);

  float* center = this->CellCenters->GetPointer(0);
  double dcenter[3];
  double* weights = new double[this->Input->GetMaxCellSize()]; // Dummy array.

  for (svtkIdType i = 0; i < numcells; i++)
  {
    svtkCell* cell = this->Input->GetCell(i);
    double pcenter[3];
    int subId;
    subId = cell->GetParametricCenter(pcenter);
    cell->EvaluateLocation(subId, pcenter, dcenter, weights);
    center[0] = dcenter[0];
    center[1] = dcenter[1];
    center[2] = dcenter[2];
    center += 3;
  }

  delete[] weights;
}

void svtkCellCenterDepthSort::ComputeDepths()
{
  float* vector = this->ComputeProjectionVector();
  svtkIdType numcells = this->Input->GetNumberOfCells();

  float* center = this->CellCenters->GetPointer(0);
  float* depth = this->CellDepths->GetPointer(0);
  for (svtkIdType i = 0; i < numcells; i++)
  {
    *(depth++) = svtkMath::Dot(center, vector);
    center += 3;
  }
}

void svtkCellCenterDepthSort::InitTraversal()
{
  svtkDebugMacro("InitTraversal");

  svtkIdType numcells = this->Input->GetNumberOfCells();

  if ((this->LastSortTime < this->Input->GetMTime()) || (this->LastSortTime < this->MTime))
  {
    svtkDebugMacro("Building cell centers array.");

    // Data may have changed.  Recompute cell centers.
    this->ComputeCellCenters();
    this->CellDepths->SetNumberOfTuples(numcells);
    this->SortedCells->SetNumberOfTuples(numcells);
  }

  svtkDebugMacro("Filling SortedCells to initial values.");
  svtkIdType* id = this->SortedCells->GetPointer(0);
  for (svtkIdType i = 0; i < numcells; i++)
  {
    *(id++) = i;
  }

  svtkDebugMacro("Calculating depths.");
  this->ComputeDepths();

  while (!this->ToSort->Stack.empty())
    this->ToSort->Stack.pop();
  this->ToSort->Stack.push(svtkIdPair(0, numcells));

  this->LastSortTime.Modified();
}

svtkIdTypeArray* svtkCellCenterDepthSort::GetNextCells()
{
  if (this->ToSort->Stack.empty())
  {
    // Already sorted and returned everything.
    return nullptr;
  }

  svtkIdType* cellIds = this->SortedCells->GetPointer(0);
  float* cellDepths = this->CellDepths->GetPointer(0);
  svtkIdPair partition;

  partition = this->ToSort->Stack.top();
  this->ToSort->Stack.pop();
  while (partition.second - partition.first > this->MaxCellsReturned)
  {
    svtkIdType left = partition.first;
    svtkIdType right = partition.second - 1;
    float pivot = cellDepths[static_cast<svtkIdType>(svtkMath::Random(left, right))];
    while (left <= right)
    {
      while ((left <= right) && (cellDepths[left] < pivot))
        left++;
      while ((left <= right) && (cellDepths[right] > pivot))
        right--;

      if (left > right)
        break;

      std::swap(cellIds[left], cellIds[right]);
      std::swap(cellDepths[left], cellDepths[right]);

      left++;
      right--;
    }

    this->ToSort->Stack.push(svtkIdPair(left, partition.second));
    partition.second = left;
  }

  if (partition.second <= partition.first)
  {
    // Got a partition of zero size.  Just recurse to get the next one.
    return this->GetNextCells();
  }

  svtkIdType firstcell = partition.first;
  svtkIdType numcells = partition.second - partition.first;

  this->SortedCellPartition->SetArray(cellIds + firstcell, numcells, 1);
  this->SortedCellPartition->SetNumberOfTuples(numcells);
  this->CellPartitionDepths->SetArray(cellDepths + firstcell, numcells, 1);
  this->CellPartitionDepths->SetNumberOfTuples(numcells);

  svtkSortDataArray::Sort(this->CellPartitionDepths, this->SortedCellPartition);
  return this->SortedCellPartition;
}
