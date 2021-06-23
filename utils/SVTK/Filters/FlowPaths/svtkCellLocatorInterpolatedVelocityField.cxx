/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellLocatorInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCellLocatorInterpolatedVelocityField.h"

#include "svtkAbstractCellLocator.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkGenericCell.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStaticCellLocator.h"

svtkStandardNewMacro(svtkCellLocatorInterpolatedVelocityField);
svtkCxxSetObjectMacro(
  svtkCellLocatorInterpolatedVelocityField, CellLocatorPrototype, svtkAbstractCellLocator);

//----------------------------------------------------------------------------
typedef std::vector<svtkSmartPointer<svtkAbstractCellLocator> > CellLocatorsTypeBase;
class svtkCellLocatorInterpolatedVelocityFieldCellLocatorsType : public CellLocatorsTypeBase
{
};

//----------------------------------------------------------------------------
svtkCellLocatorInterpolatedVelocityField::svtkCellLocatorInterpolatedVelocityField()
{
  this->LastCellLocator = nullptr;
  this->CellLocatorPrototype = nullptr;
  this->CellLocators = new svtkCellLocatorInterpolatedVelocityFieldCellLocatorsType;
}

//----------------------------------------------------------------------------
svtkCellLocatorInterpolatedVelocityField::~svtkCellLocatorInterpolatedVelocityField()
{
  this->LastCellLocator = nullptr;
  this->SetCellLocatorPrototype(nullptr);

  delete this->CellLocators;
  this->CellLocators = nullptr;
}

//----------------------------------------------------------------------------
void svtkCellLocatorInterpolatedVelocityField::SetLastCellId(svtkIdType c, int dataindex)
{
  this->LastCellId = c;
  this->LastDataSet = (*this->DataSets)[dataindex];
  this->LastCellLocator = (*this->CellLocators)[dataindex];
  this->LastDataSetIndex = dataindex;

  // If the dataset changes, then the cached cell is invalidated. We might as
  // well prefetch the cached cell either way.
  if (this->LastCellId != -1)
  {
    this->LastDataSet->GetCell(this->LastCellId, this->GenCell);
  }
}

//----------------------------------------------------------------------------
int svtkCellLocatorInterpolatedVelocityField::FunctionValues(double* x, double* f)
{
  svtkDataSet* vds = nullptr;
  svtkAbstractCellLocator* loc = nullptr;

  if (!this->LastDataSet && !this->DataSets->empty())
  {
    vds = (*this->DataSets)[0];
    loc = (*this->CellLocators)[0];
    this->LastDataSet = vds;
    this->LastCellLocator = loc;
    this->LastDataSetIndex = 0;
  }
  else
  {
    vds = this->LastDataSet;
    loc = this->LastCellLocator;
  }

  int retVal;
  if (loc)
  {
    // resort to svtkAbstractCellLocator::FindCell()
    retVal = this->FunctionValues(vds, loc, x, f);
  }
  else
  {
    // turn to svtkImageData/svtkRectilinearGrid::FindCell()
    retVal = this->FunctionValues(vds, x, f);
  }

  if (!retVal)
  {
    for (this->LastDataSetIndex = 0;
         this->LastDataSetIndex < static_cast<int>(this->DataSets->size());
         this->LastDataSetIndex++)
    {
      vds = this->DataSets->operator[](this->LastDataSetIndex);
      loc = this->CellLocators->operator[](this->LastDataSetIndex);
      if (vds && vds != this->LastDataSet)
      {
        this->ClearLastCellId();

        if (loc)
        {
          // resort to svtkAbstractCellLocator::FindCell()
          retVal = this->FunctionValues(vds, loc, x, f);
        }
        else
        {
          // turn to svtkImageData/svtkRectilinearGrid::FindCell()
          retVal = this->FunctionValues(vds, x, f);
        }

        if (retVal)
        {
          this->LastDataSet = vds;
          this->LastCellLocator = loc;
          vds = nullptr;
          loc = nullptr;
          return retVal;
        }
      }
    }

    this->LastCellId = -1;
    this->LastDataSet = (*this->DataSets)[0];
    this->LastCellLocator = (*this->CellLocators)[0];
    this->LastDataSetIndex = 0;
    vds = nullptr;
    loc = nullptr;
    return 0;
  }

  vds = nullptr;
  loc = nullptr;
  return retVal;
}

//----------------------------------------------------------------------------
int svtkCellLocatorInterpolatedVelocityField::FunctionValues(
  svtkDataSet* dataset, svtkAbstractCellLocator* loc, double* x, double* f)
{
  f[0] = f[1] = f[2] = 0.0;
  svtkDataArray* vectors = nullptr;

  if (!dataset || !loc || !dataset->IsA("svtkPointSet") ||
    !(vectors = dataset->GetPointData()->GetVectors(this->VectorsSelection)))
  {
    svtkErrorMacro(<< "Can't evaluate dataset!");
    vectors = nullptr;
    return 0;
  }

  int i;
  int subIdx;
  int numPts;
  int pntIdx;
  int bFound = 0;
  double vector[3];
  double dstns2 = 0.0;
  double toler2 = dataset->GetLength() * svtkCellLocatorInterpolatedVelocityField::TOLERANCE_SCALE;

  // check if the point is in the cached cell AND can be successfully evaluated
  if (this->LastCellId != -1 &&
    this->GenCell->EvaluatePosition(x, nullptr, subIdx, this->LastPCoords, dstns2, this->Weights) ==
      1)
  {
    bFound = 1;
    this->CacheHit++;
  }

  if (!bFound)
  {
    // cache missing or evaluation failure and then we have to find the cell
    this->CacheMiss += !(!(this->LastCellId + 1));
    this->LastCellId = loc->FindCell(x, toler2, this->GenCell, this->LastPCoords, this->Weights);
    bFound = !(!(this->LastCellId + 1));
  }

  // interpolate vectors if possible
  if (bFound)
  {
    numPts = this->GenCell->GetNumberOfPoints();
    for (i = 0; i < numPts; i++)
    {
      pntIdx = this->GenCell->PointIds->GetId(i);
      vectors->GetTuple(pntIdx, vector);
      f[0] += vector[0] * this->Weights[i];
      f[1] += vector[1] * this->Weights[i];
      f[2] += vector[2] * this->Weights[i];
    }

    if (this->NormalizeVector == true)
    {
      svtkMath::Normalize(f);
    }
  }

  vectors = nullptr;
  return bFound;
}

//----------------------------------------------------------------------------
void svtkCellLocatorInterpolatedVelocityField::AddDataSet(svtkDataSet* dataset)
{
  if (!dataset)
  {
    svtkErrorMacro(<< "Dataset nullptr!");
    return;
  }

  // insert the dataset (do NOT register the dataset to 'this')
  this->DataSets->push_back(dataset);

  // We need to attach a valid svtkAbstractCellLocator to any svtkPointSet for
  // robust cell location as svtkPointSet::FindCell() may incur failures. For
  // any non-svtkPointSet dataset, either svtkImageData or svtkRectilinearGrid,
  // we do not need to associate a svtkAbstractCellLocator with it (though a
  // nullptr svtkAbstractCellLocator is still inserted to this->CellLocators to
  // enable proper access to those valid cell locators) since these two kinds
  // of datasets themselves are able to guarantee robust as well as fast cell
  // location via svtkImageData/svtkRectilinearGrid::FindCell().
  svtkSmartPointer<svtkAbstractCellLocator> locator = nullptr; // MUST inited with 0
  if (dataset->IsA("svtkPointSet"))
  {

    if (!this->CellLocatorPrototype)
    {
      locator = svtkSmartPointer<svtkStaticCellLocator>::New();
    }
    else
    {
      locator.TakeReference(this->CellLocatorPrototype->NewInstance());
    }

    locator->SetLazyEvaluation(1);
    locator->SetDataSet(dataset);
  }
  this->CellLocators->push_back(locator);

  int size = dataset->GetMaxCellSize();
  if (size > this->WeightsSize)
  {
    this->WeightsSize = size;
    delete[] this->Weights;
    this->Weights = new double[size];
  }
}

//----------------------------------------------------------------------------
void svtkCellLocatorInterpolatedVelocityField::CopyParameters(
  svtkAbstractInterpolatedVelocityField* from)
{
  svtkAbstractInterpolatedVelocityField::CopyParameters(from);

  if (from->IsA("svtkCellLocatorInterpolatedVelocityField"))
  {
    this->SetCellLocatorPrototype(
      svtkCellLocatorInterpolatedVelocityField::SafeDownCast(from)->GetCellLocatorPrototype());
  }
}

//----------------------------------------------------------------------------
void svtkCellLocatorInterpolatedVelocityField::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CellLocators: " << this->CellLocators << endl;
  if (this->CellLocators)
  {
    os << indent << "Number of Cell Locators: " << this->CellLocators->size();
  }
  os << indent << "LastCellLocator: " << this->LastCellLocator << endl;
  os << indent << "CellLocatorPrototype: " << this->CellLocatorPrototype << endl;
}
