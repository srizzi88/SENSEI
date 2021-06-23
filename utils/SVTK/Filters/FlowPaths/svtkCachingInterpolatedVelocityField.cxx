/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCachingInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCachingInterpolatedVelocityField.h"

#include "svtkCellLocator.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkGenericCell.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"

#include <vector>

//---------------------------------------------------------------------------
svtkStandardNewMacro(svtkCachingInterpolatedVelocityField);
//---------------------------------------------------------------------------
const double IVFDataSetInfo::TOLERANCE_SCALE = 1.0E-8;
//---------------------------------------------------------------------------
IVFDataSetInfo::IVFDataSetInfo()
{
  this->VelocityFloat = nullptr;
  this->VelocityDouble = nullptr;
  this->DataSet = nullptr;
  this->Cell = nullptr;
  this->BSPTree = nullptr;
  this->Tolerance = 0.0;
  this->StaticDataSet = false;
}
//---------------------------------------------------------------------------
void IVFDataSetInfo::SetDataSet(
  svtkDataSet* data, char* velocity, bool staticdataset, svtkAbstractCellLocator* locator)
{
  this->VelocityFloat = nullptr;
  this->VelocityDouble = nullptr;
  this->DataSet = data;
  this->Cell = svtkSmartPointer<svtkGenericCell>::New();
  this->StaticDataSet = staticdataset;
  if (locator)
  {
    this->BSPTree = locator;
  }
  else if (this->DataSet->IsA("svtkUnstructuredGrid"))
  {
    if (!this->BSPTree)
    {
      this->BSPTree = svtkSmartPointer<svtkCellLocator>::New();
    }
    this->BSPTree->SetLazyEvaluation(1);
    this->BSPTree->SetDataSet(this->DataSet);
    this->BSPTree->SetUseExistingSearchStructure(this->StaticDataSet);
  }

  this->Tolerance = this->DataSet->GetLength() * IVFDataSetInfo::TOLERANCE_SCALE;
  //
  svtkDataArray* vectors = this->DataSet->GetPointData()->GetArray(velocity);
  if (svtkArrayDownCast<svtkFloatArray>(vectors))
  {
    this->VelocityFloat = svtkArrayDownCast<svtkFloatArray>(vectors)->GetPointer(0);
  }
  else if (svtkArrayDownCast<svtkDoubleArray>(vectors))
  {
    this->VelocityDouble = svtkArrayDownCast<svtkDoubleArray>(vectors)->GetPointer(0);
  }
  else
  {
    svtkGenericWarningMacro("We only support float/double velocity vectors at the current time");
  }
}
//---------------------------------------------------------------------------
IVFDataSetInfo::IVFDataSetInfo(const IVFDataSetInfo& ivfci)
{
  this->VelocityFloat = ivfci.VelocityFloat;
  this->VelocityDouble = ivfci.VelocityDouble;
  this->DataSet = ivfci.DataSet;
  this->Cell = ivfci.Cell;
  this->BSPTree = ivfci.BSPTree;
  this->Tolerance = ivfci.Tolerance;
  this->StaticDataSet = ivfci.StaticDataSet;
  this->PCoords[0] = ivfci.PCoords[0];
  this->PCoords[1] = ivfci.PCoords[1];
  this->PCoords[2] = ivfci.PCoords[2];
}
//---------------------------------------------------------------------------
IVFDataSetInfo& IVFDataSetInfo::operator=(const IVFDataSetInfo& ivfci)
{
  this->VelocityFloat = ivfci.VelocityFloat;
  this->VelocityDouble = ivfci.VelocityDouble;
  this->DataSet = ivfci.DataSet;
  this->Cell = ivfci.Cell;
  this->BSPTree = ivfci.BSPTree;
  this->Tolerance = ivfci.Tolerance;
  this->StaticDataSet = ivfci.StaticDataSet;
  this->PCoords[0] = ivfci.PCoords[0];
  this->PCoords[1] = ivfci.PCoords[1];
  this->PCoords[2] = ivfci.PCoords[2];
  return *this;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
svtkCachingInterpolatedVelocityField::svtkCachingInterpolatedVelocityField()
{
  this->NumFuncs = 3;     // u, v, w
  this->NumIndepVars = 4; // x, y, z, t
  this->VectorsSelection = nullptr;
  this->TempCell = svtkGenericCell::New();
  this->CellCacheHit = 0;
  this->DataSetCacheHit = 0;
  this->CacheMiss = 0;
  this->LastCacheIndex = 0;
  this->Cache = nullptr;
  this->LastCellId = -1;
}
//---------------------------------------------------------------------------
svtkCachingInterpolatedVelocityField::~svtkCachingInterpolatedVelocityField()
{
  this->NumFuncs = 0;
  this->NumIndepVars = 0;
  this->TempCell->Delete();
  this->SetVectorsSelection(nullptr);
}
//---------------------------------------------------------------------------
void svtkCachingInterpolatedVelocityField::SetDataSet(
  int I, svtkDataSet* dataset, bool staticdataset, svtkAbstractCellLocator* locator)
{
  int N = svtkMath::Max(I + 1, static_cast<int>(this->CacheList.size()));
  this->CacheList.resize(N);
  this->CacheList[I].SetDataSet(dataset, this->VectorsSelection, staticdataset, locator);
  //
  int maxsize = svtkMath::Max(static_cast<int>(this->Weights.size()), dataset->GetMaxCellSize());
  this->Weights.assign(maxsize, 0.0);
}
//---------------------------------------------------------------------------
void svtkCachingInterpolatedVelocityField::SetLastCellInfo(svtkIdType c, int datasetindex)
{
  if ((this->LastCacheIndex != datasetindex) || (this->LastCellId != c))
  {
    assert(this->CacheList.size() > static_cast<size_t>(datasetindex));
    this->LastCacheIndex = datasetindex;
    this->Cache = &this->CacheList[this->LastCacheIndex];
    this->LastCellId = c;
    // if the dataset changes, then the cached cell is invalidated
    // we might as well prefetch the new cached cell - we'll need it on the next test anyway
    if (this->LastCellId != -1)
    {
      assert(c < this->Cache->DataSet->GetNumberOfCells());
      this->Cache->DataSet->GetCell(this->LastCellId, this->Cache->Cell);
    }
  }
}
//---------------------------------------------------------------------------
void svtkCachingInterpolatedVelocityField::ClearLastCellInfo()
{
  this->Cache = nullptr;
  this->LastCellId = -1;
}
//---------------------------------------------------------------------------
svtkGenericCell* svtkCachingInterpolatedVelocityField::GetLastCell()
{
  if (this->Cache)
  {
    return this->Cache->Cell;
  }
  return nullptr;
}
//---------------------------------------------------------------------------
// Evaluate {u,v,w} at {x,y,z,t}
int svtkCachingInterpolatedVelocityField::FunctionValues(double* x, double* f)
{
  // Test using whatever cached information we have
  if (this->Cache)
  {
    if (this->FunctionValues(this->Cache, x, f))
    {
      this->DataSetCacheHit++;
      return 1;
    }
  }
  // don't reset this->Cache as we don't want to test it again,
  // see == comparison below

  int oldCacheIndex = this->LastCacheIndex;
  // now try each of the datasets in turn
  for (this->LastCacheIndex = 0; this->LastCacheIndex < static_cast<int>(this->CacheList.size());
       this->LastCacheIndex++)
  {
    IVFDataSetInfo* data = &this->CacheList[this->LastCacheIndex];
    if (data != this->Cache)
    {
      this->LastCellId = -1;
      if (this->FunctionValues(data, x, f))
      {
        this->Cache = data;
        this->CacheMiss++;
        return 1;
      }
    }
  }
  // failed, so clear data and set the cache index to something sensible
  this->CacheMiss++;
  this->ClearLastCellInfo();
  this->LastCacheIndex = oldCacheIndex;
  return 0;
}
//---------------------------------------------------------------------------
// same as FunctionValues, but only testing in/out of cells
int svtkCachingInterpolatedVelocityField::InsideTest(double* x)
{
  // Test using whatever cached information we have
  if (this->Cache)
  {
    // check the last cell
    int subId;
    if (this->LastCellId != -1 &&
      this->Cache->Cell->EvaluatePosition(
        x, nullptr, subId, this->Cache->PCoords, this->Cache->Tolerance, &this->Weights[0]) == 1)
    {
      return 1;
    }
    // check this dataset
    if (this->InsideTest(this->Cache, x))
    {
      return 1;
    }
  }
  // don't reset this->Cache as we don't want to test it again,
  // see != comparison below

  // now try each of the other datasets in turn
  for (this->LastCacheIndex = 0; this->LastCacheIndex < static_cast<int>(this->CacheList.size());
       this->LastCacheIndex++)
  {
    IVFDataSetInfo* data = &this->CacheList[this->LastCacheIndex];
    if (data != this->Cache)
    {
      this->LastCellId = -1;
      if (this->InsideTest(data, x))
      {
        this->Cache = data;
        return 1;
      }
    }
  }
  // failed, so clear data
  this->ClearLastCellInfo();
  return 0;
}
//---------------------------------------------------------------------------
int svtkCachingInterpolatedVelocityField::InsideTest(IVFDataSetInfo* data, double* x)
{
  int cellId =
    data->BSPTree->FindCell(x, data->Tolerance, data->Cell, data->PCoords, &this->Weights[0]);
  if (cellId != -1)
  {
    this->LastCellId = cellId;
    return 1;
  }
  return 0;
}
//---------------------------------------------------------------------------
// Evaluate {u,v,w} at {x,y,z,t}
int svtkCachingInterpolatedVelocityField::FunctionValues(IVFDataSetInfo* data, double* x, double* f)
{
  int subId;
  double dist2;

  if (this->LastCellId >= 0)
  {
    bool inbox = true;
    if (data->BSPTree && !data->BSPTree->InsideCellBounds(x, this->LastCellId))
    {
      inbox = false;
    }
    if (inbox &&
      data->Cell->EvaluatePosition(x, nullptr, subId, data->PCoords, dist2, &this->Weights[0]) == 1)
    {
      this->FastCompute(data, f);
      this->CellCacheHit++;
      return 1;
    }
  }

  // we need to search the whole dataset
  if (data->BSPTree)
  {
    int cellId =
      data->BSPTree->FindCell(x, data->Tolerance, data->Cell, data->PCoords, &this->Weights[0]);
    this->LastCellId = cellId;
  }
  else
  {
    svtkGenericCell* tmpCell = nullptr;
    if (this->LastCellId >= 0)
    {
      data->DataSet->GetCell(this->LastCellId, this->TempCell);
      tmpCell = this->TempCell;
    }
    this->LastCellId = data->DataSet->FindCell(x, tmpCell, data->Cell, this->LastCellId,
      data->Tolerance, subId, data->PCoords, &this->Weights[0]);
    if (this->LastCellId != -1)
    {
      data->DataSet->GetCell(this->LastCellId, data->Cell);
    }
  }

  // if the cell is valid
  if (this->LastCellId != -1)
  {
    this->FastCompute(data, f);
  }
  else
  {
    return 0;
  }

  return 1;
}
//---------------------------------------------------------------------------
void svtkCachingInterpolatedVelocityField::FastCompute(IVFDataSetInfo* data, double f[3])
{
  f[0] = f[1] = f[2] = 0.0;
  int numPts = data->Cell->GetNumberOfPoints();
  // interpolate the vectors
  double* dvectors = data->VelocityDouble;
  if (dvectors)
  {
    for (int j = 0; j < numPts; j++)
    {
      svtkIdType id = data->Cell->PointIds->GetId(j);
      f[0] += dvectors[id * 3 + 0] * this->Weights[j];
      f[1] += dvectors[id * 3 + 1] * this->Weights[j];
      f[2] += dvectors[id * 3 + 2] * this->Weights[j];
    }
  }
  else
  {
    float* fvectors = data->VelocityFloat;
    for (int j = 0; j < numPts; j++)
    {
      svtkIdType id = data->Cell->PointIds->GetId(j);
      f[0] += fvectors[id * 3 + 0] * this->Weights[j];
      f[1] += fvectors[id * 3 + 1] * this->Weights[j];
      f[2] += fvectors[id * 3 + 2] * this->Weights[j];
    }
  }
}
//---------------------------------------------------------------------------
bool svtkCachingInterpolatedVelocityField::InterpolatePoint(svtkPointData* outPD, svtkIdType outIndex)
{
  if (!this->Cache || !this->Cache->DataSet)
  {
    return 0;
  }
  outPD->InterpolatePoint(
    this->Cache->DataSet->GetPointData(), outIndex, this->Cache->Cell->PointIds, &this->Weights[0]);
  return 1;
}
//---------------------------------------------------------------------------
bool svtkCachingInterpolatedVelocityField::InterpolatePoint(
  svtkCachingInterpolatedVelocityField* inCIVF, svtkPointData* outPD, svtkIdType outIndex)
{
  if (!this->Cache || !this->Cache->DataSet)
  {
    return 0;
  }
  svtkPointData* inPD = inCIVF->Cache->DataSet->GetPointData();
  outPD->InterpolatePoint(inPD, outIndex, this->Cache->Cell->PointIds, &this->Weights[0]);
  return 1;
}
//---------------------------------------------------------------------------
int svtkCachingInterpolatedVelocityField::GetLastWeights(double* w)
{
  // If last cell is valid, fill w with the interpolation weights
  // and return true
  if (this->Cache && this->LastCellId >= 0)
  {
    int numPts = this->Cache->Cell->GetNumberOfPoints();
    for (int j = 0; j < numPts; j++)
    {
      w[j] = this->Weights[j];
    }
    return 1;
  }
  // otherwise, return false
  else
  {
    return 0;
  }
}
//---------------------------------------------------------------------------
int svtkCachingInterpolatedVelocityField::GetLastLocalCoordinates(double pcoords[3])
{
  // If last cell is valid, fill p with the local coordinates
  // and return true
  if (this->Cache && this->LastCellId >= 0)
  {
    for (int j = 0; j < 3; j++)
    {
      pcoords[j] = this->Cache->PCoords[j];
    }
    return 1;
  }
  // otherwise, return false
  else
  {
    return 0;
  }
}
//---------------------------------------------------------------------------
void svtkCachingInterpolatedVelocityField::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (!Weights.empty())
  {
    os << indent << "Weights: " << &this->Weights[0] << endl;
  }
  else
  {
    os << indent << "Weights: (none)" << endl;
  }

  os << indent << "Cell Cache hit: " << this->CellCacheHit << endl;
  os << indent << "DataSet Cache hit: " << this->DataSetCacheHit << endl;
  os << indent << "Cache miss: " << this->CacheMiss << endl;
  os << indent
     << "VectorsSelection: " << (this->VectorsSelection ? this->VectorsSelection : "(none)")
     << endl;

  if (this->Cache)
  {
    os << indent << "Cache->DataSet : " << this->Cache->DataSet << endl;
  }
  else
  {
    os << indent << "Cache->DataSet : (none)" << endl;
  }

  os << indent << "LastCacheIndex : " << this->LastCacheIndex << endl;
}
//---------------------------------------------------------------------------
