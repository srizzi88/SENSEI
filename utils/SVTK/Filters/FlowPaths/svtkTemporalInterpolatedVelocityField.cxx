/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalInterpolatedVelocityField.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "svtkTemporalInterpolatedVelocityField.h"

#include "svtkAbstractCellLocator.h"
#include "svtkCachingInterpolatedVelocityField.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

#include <vector>
//---------------------------------------------------------------------------
svtkStandardNewMacro(svtkTemporalInterpolatedVelocityField);
//---------------------------------------------------------------------------
svtkTemporalInterpolatedVelocityField::svtkTemporalInterpolatedVelocityField()
{
  this->NumFuncs = 3;     // u, v, w
  this->NumIndepVars = 4; // x, y, z, t
  this->IVF[0] = svtkSmartPointer<svtkCachingInterpolatedVelocityField>::New();
  this->IVF[1] = svtkSmartPointer<svtkCachingInterpolatedVelocityField>::New();
  this->LastGoodVelocity[0] = 0.0;
  this->LastGoodVelocity[1] = 0.0;
  this->LastGoodVelocity[2] = 0.0;
  this->CurrentWeight = 0.0;
  this->OneMinusWeight = 1.0;
  this->ScaleCoeff = 1.0;

  this->Vals1[0] = this->Vals1[1] = this->Vals1[2] = 0.0;
  this->Vals2[0] = this->Vals2[1] = this->Vals2[2] = 0.0;
  this->Times[0] = this->Times[1] = 0.0;
}

//---------------------------------------------------------------------------
svtkTemporalInterpolatedVelocityField::~svtkTemporalInterpolatedVelocityField()
{
  this->NumFuncs = 0;
  this->NumIndepVars = 0;
  this->SetVectorsSelection(nullptr);
  this->IVF[0] = nullptr;
  this->IVF[1] = nullptr;
}
//---------------------------------------------------------------------------
void svtkTemporalInterpolatedVelocityField::SetDataSetAtTime(
  int I, int N, double T, svtkDataSet* dataset, bool staticdataset)
{
  this->Times[N] = T;
  if ((this->Times[1] - this->Times[0]) > 0)
  {
    this->ScaleCoeff = 1.0 / (this->Times[1] - this->Times[0]);
  }
  if (N == 0)
  {
    this->IVF[N]->SetDataSet(I, dataset, staticdataset, nullptr);
  }
  // when the datasets for the second time set are added, set the static flag
  if (N == 1)
  {
    bool is_static = staticdataset && this->IVF[0]->CacheList[I].StaticDataSet;
    if (static_cast<size_t>(I) >= this->StaticDataSets.size())
    {
      this->StaticDataSets.resize(I + 1, is_static);
    }
    if (is_static)
    {
      this->IVF[N]->SetDataSet(I, dataset, staticdataset, this->IVF[0]->CacheList[I].BSPTree);
    }
    else
    {
      this->IVF[N]->SetDataSet(I, dataset, staticdataset, nullptr);
    }
  }
}
//---------------------------------------------------------------------------
bool svtkTemporalInterpolatedVelocityField::IsStatic(int datasetIndex)
{
  return this->StaticDataSets[datasetIndex];
}
//---------------------------------------------------------------------------
void svtkTemporalInterpolatedVelocityField::SetVectorsSelection(const char* v)
{
  this->IVF[0]->SelectVectors(v);
  this->IVF[1]->SelectVectors(v);
}
//---------------------------------------------------------------------------
void svtkTemporalInterpolatedVelocityField::ClearCache()
{
  this->IVF[0]->SetLastCellInfo(-1, 0);
  this->IVF[1]->SetLastCellInfo(-1, 0);
}
//---------------------------------------------------------------------------
void svtkTemporalInterpolatedVelocityField::SetCachedCellIds(svtkIdType id[2], int ds[2])
{
  if (id[0] != -1)
  {
    this->IVF[0]->SetLastCellInfo(id[0], ds[0]);
  }
  else
  {
    this->IVF[0]->SetLastCellInfo(-1, 0);
  }
  //
  if (id[1] != -1)
  {
    this->IVF[1]->SetLastCellInfo(id[1], ds[1]);
  }
  else
  {
    this->IVF[1]->SetLastCellInfo(-1, 0);
  }
}
//---------------------------------------------------------------------------
bool svtkTemporalInterpolatedVelocityField::GetCachedCellIds(svtkIdType id[2], int ds[2])
{
  id[0] = this->IVF[0]->LastCellId;
  ds[0] = (id[0] == -1) ? 0 : this->IVF[0]->LastCacheIndex;
  //
  id[1] = this->IVF[1]->LastCellId;
  ds[1] = (id[1] == -1) ? 0 : this->IVF[1]->LastCacheIndex;
  return ((id[0] >= 0) && (id[1] >= 0));
}
//---------------------------------------------------------------------------
void svtkTemporalInterpolatedVelocityField::AdvanceOneTimeStep()
{
  for (unsigned int i = 0; i < this->IVF[0]->CacheList.size(); i++)
  {
    if (this->IsStatic(i))
    {
      this->IVF[0]->ClearLastCellInfo();
      this->IVF[1]->ClearLastCellInfo();
    }
    else
    {
      this->IVF[0] = this->IVF[1];
      this->IVF[1] = svtkSmartPointer<svtkCachingInterpolatedVelocityField>::New();
    }
  }
}
//---------------------------------------------------------------------------
void svtkTemporalInterpolatedVelocityField::ShowCacheResults()
{
  svtkErrorMacro(<< ")\n"
                << "T0 - (cell hit : " << this->IVF[0]->CellCacheHit
                << "  (dataset hit : " << this->IVF[0]->DataSetCacheHit - this->IVF[0]->CellCacheHit
                << "         (miss : " << this->IVF[0]->CacheMiss << ")\n"
                << "T1 - (cell hit : " << this->IVF[1]->CellCacheHit
                << "  (dataset hit : " << this->IVF[1]->DataSetCacheHit - this->IVF[1]->CellCacheHit
                << "         (miss : " << this->IVF[1]->CacheMiss);
}
//---------------------------------------------------------------------------
static double svtkTIVFWeightTolerance = 1E-3;
// Evaluate u,v,w at x,y,z,t
int svtkTemporalInterpolatedVelocityField::TestPoint(double* x)
{
  this->CurrentWeight = (x[3] - this->Times[0]) * this->ScaleCoeff;
  this->OneMinusWeight = 1.0 - this->CurrentWeight;
  if (this->CurrentWeight < (0.0 + svtkTIVFWeightTolerance))
  {
    this->CurrentWeight = 0.0;
  }
  if (this->CurrentWeight > (1.0 - svtkTIVFWeightTolerance))
  {
    this->CurrentWeight = 1.0;
  }
  //
  // are we inside dataset at T0
  //
  if (this->IVF[0]->FunctionValues(x, this->Vals1))
  {
    // if we are inside at T0 and static, we must be inside at T1
    if (this->IsStatic(this->IVF[0]->LastCacheIndex))
    {
      // compute using weights from dataset 0 and vectors from dataset 1
      this->IVF[1]->SetLastCellInfo(this->IVF[0]->LastCellId, this->IVF[0]->LastCacheIndex);
      this->IVF[0]->FastCompute(this->IVF[1]->Cache, this->Vals2);
      for (int i = 0; i < this->NumFuncs; i++)
      {
        this->LastGoodVelocity[i] =
          this->OneMinusWeight * this->Vals1[i] + this->CurrentWeight * this->Vals2[i];
      }
      return ID_INSIDE_ALL;
    }
    // dynamic, we need to test at T1
    if (!this->IVF[1]->FunctionValues(x, this->Vals2))
    {
      // inside at T0, but outside at T1, return velocity for T0
      for (int i = 0; i < this->NumFuncs; i++)
      {
        this->LastGoodVelocity[i] = this->Vals1[i];
      }
      return ID_OUTSIDE_T1;
    }
    // both valid, compute correct value
    for (int i = 0; i < this->NumFuncs; i++)
    {
      this->LastGoodVelocity[i] =
        this->OneMinusWeight * this->Vals1[i] + this->CurrentWeight * this->Vals2[i];
    }
    return ID_INSIDE_ALL;
  }
  // Outside at T0, either abort or use T1
  // if we are outside at T0 and static, we must be outside at T1
  if (this->IsStatic(this->IVF[0]->LastCacheIndex))
  {
    return ID_OUTSIDE_ALL;
  }
  // we are dynamic, so test T1
  if (this->IVF[1]->FunctionValues(x, this->Vals2))
  {
    // inside at T1, but outside at T0, return velocity for T1
    for (int i = 0; i < this->NumFuncs; i++)
    {
      this->LastGoodVelocity[i] = this->Vals2[i];
    }
    return ID_OUTSIDE_T0;
  }
  // failed both, so exit
  return ID_OUTSIDE_ALL;
}
//---------------------------------------------------------------------------
// Evaluate u,v,w at x,y,z,t
int svtkTemporalInterpolatedVelocityField::QuickTestPoint(double* x)
{
  // if outside, return 0
  if (!this->IVF[0]->InsideTest(x))
  {
    return 0;
  }
  // if inside and static dataset hit, skip next test
  if (!this->IsStatic(this->IVF[0]->LastCacheIndex))
  {
    if (!this->IVF[1]->InsideTest(x))
    {
      return 0;
    }
  }
  return 1;
}
//---------------------------------------------------------------------------
// Evaluate u,v,w at x,y,z,t
int svtkTemporalInterpolatedVelocityField::FunctionValues(double* x, double* u)
{
  if (this->TestPoint(x) == ID_OUTSIDE_ALL)
  {
    return 0;
  }
  for (int i = 0; i < this->NumFuncs; i++)
  {
    u[i] = this->LastGoodVelocity[i];
  }
  return 1;
}
//---------------------------------------------------------------------------
int svtkTemporalInterpolatedVelocityField::FunctionValuesAtT(int T, double* x, double* u)
{
  //
  // Try velocity at T0
  //
  if (T == 0)
  {
    if (!this->IVF[0]->FunctionValues(x, this->Vals1))
    {
      return 0;
    }
    for (int i = 0; i < this->NumFuncs; i++)
    {
      this->LastGoodVelocity[i] = u[i] = this->Vals1[i];
    }
    if (this->IsStatic(this->IVF[0]->LastCacheIndex))
    {
      this->IVF[1]->SetLastCellInfo(this->IVF[0]->LastCellId, this->IVF[0]->LastCacheIndex);
    }
  }
  //
  // Try velocity at T1
  //
  else if (T == 1)
  {
    if (!this->IVF[1]->FunctionValues(x, this->Vals2))
    {
      return 0;
    }
    for (int i = 0; i < this->NumFuncs; i++)
    {
      this->LastGoodVelocity[i] = u[i] = this->Vals2[i];
    }
    if (this->IsStatic(this->IVF[1]->LastCacheIndex))
    {
      this->IVF[0]->SetLastCellInfo(this->IVF[1]->LastCellId, this->IVF[1]->LastCacheIndex);
    }
  }
  return 1;
}
//---------------------------------------------------------------------------
bool svtkTemporalInterpolatedVelocityField::InterpolatePoint(
  svtkPointData* outPD1, svtkPointData* outPD2, svtkIdType outIndex)
{
  bool ok1 = this->IVF[0]->InterpolatePoint(outPD1, outIndex);
  bool ok2 = this->IVF[1]->InterpolatePoint(outPD2, outIndex);
  return (ok1 || ok2);
}
//---------------------------------------------------------------------------
bool svtkTemporalInterpolatedVelocityField::InterpolatePoint(
  int T, svtkPointData* outPD1, svtkIdType outIndex)
{
  svtkCachingInterpolatedVelocityField* inivf = this->IVF[T];
  // force use of correct weights/etc if static as only T0 are valid
  if (T == 1 && this->IsStatic(this->IVF[T]->LastCacheIndex))
  {
    T = 0;
  }
  //
  return this->IVF[T]->InterpolatePoint(inivf, outPD1, outIndex);
}
//---------------------------------------------------------------------------
bool svtkTemporalInterpolatedVelocityField::GetVorticityData(
  int T, double pcoords[3], double* weights, svtkGenericCell*& cell, svtkDoubleArray* cellVectors)
{
  // force use of correct weights/etc if static as only T0 are valid
  if (T == 1 && this->IsStatic(this->IVF[T]->LastCacheIndex))
  {
    T = 0;
  }
  //
  if (this->IVF[T]->GetLastWeights(weights) && this->IVF[T]->GetLastLocalCoordinates(pcoords) &&
    (cell = this->IVF[T]->GetLastCell()))
  {
    svtkDataSet* ds = this->IVF[T]->Cache->DataSet;
    svtkPointData* pd = ds->GetPointData();
    svtkDataArray* da = pd->GetVectors(this->IVF[T]->GetVectorsSelection());
    da->GetTuples(cell->PointIds, cellVectors);
    return 1;
  }
  return 0;
}
//---------------------------------------------------------------------------
void svtkTemporalInterpolatedVelocityField::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "LastGoodVelocity: " << this->LastGoodVelocity[0] << ", "
     << this->LastGoodVelocity[1] << ", " << this->LastGoodVelocity[2] << endl;
  os << indent << "CurrentWeight: " << this->CurrentWeight << endl;
}
//---------------------------------------------------------------------------
