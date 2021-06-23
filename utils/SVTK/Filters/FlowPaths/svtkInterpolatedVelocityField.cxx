/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInterpolatedVelocityField.h"

#include "svtkDataSet.h"
#include "svtkGenericCell.h"
#include "svtkObjectFactory.h"

//----------------------------------------------------------------------------
svtkStandardNewMacro(svtkInterpolatedVelocityField);

//----------------------------------------------------------------------------
void svtkInterpolatedVelocityField::AddDataSet(svtkDataSet* dataset)
{
  if (!dataset)
  {
    return;
  }

  // insert the dataset (do NOT register the dataset to 'this')
  this->DataSets->push_back(dataset);

  int size = dataset->GetMaxCellSize();
  if (size > this->WeightsSize)
  {
    this->WeightsSize = size;
    delete[] this->Weights;
    this->Weights = new double[size];
  }
}

//----------------------------------------------------------------------------
void svtkInterpolatedVelocityField::SetLastCellId(svtkIdType c, int dataindex)
{
  this->LastCellId = c;
  this->LastDataSet = (*this->DataSets)[dataindex];

  // if the dataset changes, then the cached cell is invalidated
  // we might as well prefetch the cached cell either way
  if (this->LastCellId != -1)
  {
    this->LastDataSet->GetCell(this->LastCellId, this->GenCell);
  }

  this->LastDataSetIndex = dataindex;
}

//----------------------------------------------------------------------------
int svtkInterpolatedVelocityField::FunctionValues(double* x, double* f)
{
  svtkDataSet* ds;
  if (!this->LastDataSet && !this->DataSets->empty())
  {
    ds = (*this->DataSets)[0];
    this->LastDataSet = ds;
    this->LastDataSetIndex = 0;
  }
  else
  {
    ds = this->LastDataSet;
  }

  int retVal = this->FunctionValues(ds, x, f);

  if (!retVal)
  {
    for (this->LastDataSetIndex = 0;
         this->LastDataSetIndex < static_cast<int>(this->DataSets->size());
         this->LastDataSetIndex++)
    {
      ds = this->DataSets->operator[](this->LastDataSetIndex);
      if (ds && ds != this->LastDataSet)
      {
        this->ClearLastCellId();
        retVal = this->FunctionValues(ds, x, f);
        if (retVal)
        {
          this->LastDataSet = ds;
          return retVal;
        }
      }
    }
    this->LastCellId = -1;
    this->LastDataSetIndex = 0;
    this->LastDataSet = (*this->DataSets)[0];
    return 0;
  }

  return retVal;
}

//----------------------------------------------------------------------------
int svtkInterpolatedVelocityField::SnapPointOnCell(double* pOrigin, double* pSnap)
{
  if (this->LastDataSet == nullptr)
  {
    return 0;
  }
  if (!this->FindAndUpdateCell(this->LastDataSet, pOrigin))
  {
    return 0;
  }
  double dist2;
  this->GenCell->EvaluatePosition(
    pOrigin, pSnap, this->LastSubId, this->LastPCoords, dist2, this->Weights);
  return 1;
}

//----------------------------------------------------------------------------
void svtkInterpolatedVelocityField::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
