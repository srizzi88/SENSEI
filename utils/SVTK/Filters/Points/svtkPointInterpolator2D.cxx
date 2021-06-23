/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointInterpolator2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointInterpolator2D.h"

#include "svtkAbstractPointLocator.h"
#include "svtkArrayListTemplate.h"
#include "svtkCellData.h"
#include "svtkCharArray.h"
#include "svtkDataArray.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStaticPointLocator.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkVoronoiKernel.h"

svtkStandardNewMacro(svtkPointInterpolator2D);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{
// Project source points onto plane
struct ProjectPoints
{
  svtkDataSet* Source;
  double* OutPoints;

  ProjectPoints(svtkDataSet* source, double* outPts)
    : Source(source)
    , OutPoints(outPts)
  {
  }

  // Threaded projection
  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    double* p = this->OutPoints + 3 * ptId;
    double x[3];
    for (; ptId < endPtId; ++ptId)
    {
      this->Source->GetPoint(ptId, x);
      *p++ = x[0];
      *p++ = x[1];
      *p++ = 0.0; // x-y projection
    }
  }
};

// Project source points onto plane
struct ProjectPointsWithScalars
{
  svtkDataSet* Source;
  double* OutPoints;
  double* ZScalars;

  ProjectPointsWithScalars(svtkDataSet* source, double* outPts, double* zScalars)
    : Source(source)
    , OutPoints(outPts)
    , ZScalars(zScalars)
  {
  }

  // Threaded projection
  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    double* p = this->OutPoints + 3 * ptId;
    double* s = this->ZScalars + ptId;
    double x[3];
    for (; ptId < endPtId; ++ptId)
    {
      this->Source->GetPoint(ptId, x);
      *p++ = x[0];
      *p++ = x[1];
      *p++ = 0.0; // x-y projection
      *s++ = x[2];
    }
  }
};

// The threaded core of the algorithm
struct ProbePoints
{
  svtkDataSet* Input;
  svtkInterpolationKernel* Kernel;
  svtkAbstractPointLocator* Locator;
  svtkPointData* InPD;
  svtkPointData* OutPD;
  ArrayList Arrays;
  char* Valid;
  int Strategy;

  // Don't want to allocate these working arrays on every thread invocation,
  // so make them thread local.
  svtkSMPThreadLocalObject<svtkIdList> PIds;
  svtkSMPThreadLocalObject<svtkDoubleArray> Weights;

  ProbePoints(svtkDataSet* input, svtkInterpolationKernel* kernel, svtkAbstractPointLocator* loc,
    svtkPointData* inPD, svtkPointData* outPD, int strategy, char* valid, double nullV)
    : Input(input)
    , Kernel(kernel)
    , Locator(loc)
    , InPD(inPD)
    , OutPD(outPD)
    , Valid(valid)
    , Strategy(strategy)
  {
    this->Arrays.AddArrays(input->GetNumberOfPoints(), inPD, outPD, nullV);
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    svtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory
    svtkDoubleArray*& weights = this->Weights.Local();
    weights->Allocate(128);
  }

  // When null point is encountered
  void AssignNullPoint(const double x[3], svtkIdList* pIds, svtkDoubleArray* weights, svtkIdType ptId)
  {
    if (this->Strategy == svtkPointInterpolator2D::MASK_POINTS)
    {
      this->Valid[ptId] = 0;
      this->Arrays.AssignNullValue(ptId);
    }
    else if (this->Strategy == svtkPointInterpolator2D::NULL_VALUE)
    {
      this->Arrays.AssignNullValue(ptId);
    }
    else // svtkPointInterpolator2D::CLOSEST_POINT:
    {
      pIds->SetNumberOfIds(1);
      svtkIdType pId = this->Locator->FindClosestPoint(x);
      pIds->SetId(0, pId);
      weights->SetNumberOfTuples(1);
      weights->SetValue(0, 1.0);
      this->Arrays.Interpolate(1, pIds->GetPointer(0), weights->GetPointer(0), ptId);
    }
  }

  // Threaded interpolation method
  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    double x[3];
    svtkIdList*& pIds = this->PIds.Local();
    svtkIdType numWeights;
    svtkDoubleArray*& weights = this->Weights.Local();

    for (; ptId < endPtId; ++ptId)
    {
      this->Input->GetPoint(ptId, x);
      x[2] = 0.0; // x-y projection

      if (this->Kernel->ComputeBasis(x, pIds) > 0)
      {
        numWeights = this->Kernel->ComputeWeights(x, pIds, weights);
        this->Arrays.Interpolate(numWeights, pIds->GetPointer(0), weights->GetPointer(0), ptId);
      }
      else
      {
        this->AssignNullPoint(x, pIds, weights, ptId);
      } // null point
    }   // for all dataset points
  }

  void Reduce() {}

}; // ProbePoints

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkPointInterpolator2D::svtkPointInterpolator2D()
{
  this->InterpolateZ = true;
  this->ZArrayName = "Elevation";
}

//----------------------------------------------------------------------------
svtkPointInterpolator2D::~svtkPointInterpolator2D() = default;

//----------------------------------------------------------------------------
// The driver of the algorithm
void svtkPointInterpolator2D::Probe(svtkDataSet* input, svtkDataSet* source, svtkDataSet* output)
{
  // Make sure there is a kernel
  if (!this->Kernel)
  {
    svtkErrorMacro(<< "Interpolation kernel required\n");
    return;
  }

  // Start by building the locator
  if (!this->Locator)
  {
    svtkErrorMacro(<< "Point locator required\n");
    return;
  }

  // We need to project the source points to the z=0.0 plane
  svtkIdType numSourcePts = source->GetNumberOfPoints();
  svtkPolyData* projSource = svtkPolyData::New();
  projSource->ShallowCopy(source);
  svtkPoints* projPoints = svtkPoints::New();
  projPoints->SetDataTypeToDouble();
  projPoints->SetNumberOfPoints(numSourcePts);
  projSource->SetPoints(projPoints);
  projPoints->UnRegister(this);
  svtkDoubleArray* zScalars = nullptr;

  // Create elevation scalars if necessary
  if (this->InterpolateZ)
  {
    zScalars = svtkDoubleArray::New();
    zScalars->SetName(this->GetZArrayName());
    zScalars->SetNumberOfTuples(numSourcePts);
    ProjectPointsWithScalars project(source, static_cast<double*>(projPoints->GetVoidPointer(0)),
      static_cast<double*>(zScalars->GetVoidPointer(0)));
    svtkSMPTools::For(0, numSourcePts, project);
    projSource->GetPointData()->AddArray(zScalars);
    zScalars->UnRegister(this);
  }
  else
  {
    ProjectPoints project(source, static_cast<double*>(projPoints->GetVoidPointer(0)));
    svtkSMPTools::For(0, numSourcePts, project);
  }

  this->Locator->SetDataSet(projSource);
  this->Locator->BuildLocator();

  // Set up the interpolation process
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkPointData* inPD = projSource->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD, numPts);

  // Masking if requested
  char* mask = nullptr;
  if (this->NullPointsStrategy == svtkPointInterpolator2D::MASK_POINTS)
  {
    this->ValidPointsMask = svtkCharArray::New();
    this->ValidPointsMask->SetNumberOfTuples(numPts);
    mask = this->ValidPointsMask->GetPointer(0);
    std::fill_n(mask, numPts, 1);
  }

  // Now loop over input points, finding closest points and invoking kernel.
  if (this->Kernel->GetRequiresInitialization())
  {
    this->Kernel->Initialize(this->Locator, source, inPD);
  }

  // If the input is image data then there is a faster path
  ProbePoints probe(input, this->Kernel, this->Locator, inPD, outPD, this->NullPointsStrategy, mask,
    this->NullValue);
  svtkSMPTools::For(0, numPts, probe);

  // Clean up
  projSource->Delete();
  if (mask)
  {
    this->ValidPointsMask->SetName(this->ValidPointsMaskArrayName);
    outPD->AddArray(this->ValidPointsMask);
    this->ValidPointsMask->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkPointInterpolator2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Interpolate Z: " << (this->InterpolateZ ? "On" : " Off") << "\n";
}
