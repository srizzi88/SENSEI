/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPCANormalEstimation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPCANormalEstimation.h"

#include "svtkAbstractPointLocator.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStaticPointLocator.h"

svtkStandardNewMacro(svtkPCANormalEstimation);
svtkCxxSetObjectMacro(svtkPCANormalEstimation, Locator, svtkAbstractPointLocator);

namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm.
template <typename T>
struct GenerateNormals
{
  const T* Points;
  svtkAbstractPointLocator* Locator;
  int SampleSize;
  float* Normals;
  int Orient;
  double OPoint[3];
  bool Flip;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage lots of new/delete.
  svtkSMPThreadLocalObject<svtkIdList> PIds;

  GenerateNormals(T* points, svtkAbstractPointLocator* loc, int sample, float* normals, int orient,
    double opoint[3], bool flip)
    : Points(points)
    , Locator(loc)
    , SampleSize(sample)
    , Normals(normals)
    , Orient(orient)
    , Flip(flip)
  {
    this->OPoint[0] = opoint[0];
    this->OPoint[1] = opoint[1];
    this->OPoint[2] = opoint[2];
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    svtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory
  }

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const T* px = this->Points + 3 * ptId;
    const T* py;
    float* n = this->Normals + 3 * ptId;
    double x[3], mean[3], o[3];
    svtkIdList*& pIds = this->PIds.Local();
    svtkIdType numPts, nei;
    int sample, i;
    double *a[3], a0[3], a1[3], a2[3], xp[3];
    a[0] = a0;
    a[1] = a1;
    a[2] = a2;
    double *v[3], v0[3], v1[3], v2[3];
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    double eVecMin[3], eVal[3];
    float flipVal = (this->Flip ? -1.0 : 1.0);

    for (; ptId < endPtId; ++ptId)
    {
      x[0] = static_cast<double>(*px++);
      x[1] = static_cast<double>(*px++);
      x[2] = static_cast<double>(*px++);

      // Retrieve the local neighborhood
      this->Locator->FindClosestNPoints(this->SampleSize, x, pIds);
      numPts = pIds->GetNumberOfIds();

      // First step: compute the mean position of the neighborhood.
      mean[0] = mean[1] = mean[2] = 0.0;
      for (sample = 0; sample < numPts; ++sample)
      {
        nei = pIds->GetId(sample);
        py = this->Points + 3 * nei;
        mean[0] += static_cast<double>(*py++);
        mean[1] += static_cast<double>(*py++);
        mean[2] += static_cast<double>(*py);
      }
      mean[0] /= static_cast<double>(numPts);
      mean[1] /= static_cast<double>(numPts);
      mean[2] /= static_cast<double>(numPts);

      // Now compute the covariance matrix
      a0[0] = a1[0] = a2[0] = 0.0;
      a0[1] = a1[1] = a2[1] = 0.0;
      a0[2] = a1[2] = a2[2] = 0.0;
      for (sample = 0; sample < numPts; ++sample)
      {
        nei = pIds->GetId(sample);
        py = this->Points + 3 * nei;
        xp[0] = static_cast<double>(*py++) - mean[0];
        xp[1] = static_cast<double>(*py++) - mean[1];
        xp[2] = static_cast<double>(*py) - mean[2];
        for (i = 0; i < 3; i++)
        {
          a0[i] += xp[0] * xp[i];
          a1[i] += xp[1] * xp[i];
          a2[i] += xp[2] * xp[i];
        }
      }
      for (i = 0; i < 3; i++)
      {
        a0[i] /= static_cast<double>(numPts);
        a1[i] /= static_cast<double>(numPts);
        a2[i] /= static_cast<double>(numPts);
      }

      // Next extract the eigenvectors and values
      svtkMath::Jacobi(a, eVal, v);
      // eVecMax[0] = v[0][0]; eVecMax[1] = v[1][0]; eVecMax[2] = v[2][0];
      // eVecMid[0] = v[0][1]; eVecMid[1] = v[1][1]; eVecMid[2] = v[2][1];
      eVecMin[0] = v[0][2];
      eVecMin[1] = v[1][2];
      eVecMin[2] = v[2][2];

      // Orient properly
      if (this->Orient == svtkPCANormalEstimation::POINT)
      {
        o[0] = this->OPoint[0] - x[0];
        o[1] = this->OPoint[1] - x[1];
        o[2] = this->OPoint[2] - x[2];
        if (svtkMath::Dot(o, eVecMin) < 0.0)
        {
          eVecMin[0] *= -1;
          eVecMin[1] *= -1;
          eVecMin[2] *= -1;
        }
      }

      // Finally compute the point normal (which is the smallest eigenvector)
      *n++ = flipVal * eVecMin[0];
      *n++ = flipVal * eVecMin[1];
      *n++ = flipVal * eVecMin[2];

    } // for all points
  }

  void Reduce() {}

  static void Execute(svtkPCANormalEstimation* self, svtkIdType numPts, T* points, float* normals,
    int orient, double opoint[3], bool flip)
  {
    GenerateNormals gen(
      points, self->GetLocator(), self->GetSampleSize(), normals, orient, opoint, flip);
    svtkSMPTools::For(0, numPts, gen);
  }
}; // GenerateNormals

} // anonymous namespace

//================= Begin SVTK class proper =======================================
//----------------------------------------------------------------------------
svtkPCANormalEstimation::svtkPCANormalEstimation()
{

  this->SampleSize = 25;
  this->Locator = svtkStaticPointLocator::New();
  this->NormalOrientation = svtkPCANormalEstimation::POINT;
  this->OrientationPoint[0] = this->OrientationPoint[1] = this->OrientationPoint[2] = 0.0;
  this->FlipNormals = false;
}

//----------------------------------------------------------------------------
svtkPCANormalEstimation::~svtkPCANormalEstimation()
{
  this->SetLocator(nullptr);
}

//----------------------------------------------------------------------------
// Produce the output data
int svtkPCANormalEstimation::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // Check the input
  if (!input || !output)
  {
    return 1;
  }
  svtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  // Start by building the locator.
  if (!this->Locator)
  {
    svtkErrorMacro(<< "Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Generate the point normals.
  svtkFloatArray* normals = svtkFloatArray::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(numPts);
  normals->SetName("PCANormals");
  float* n = static_cast<float*>(normals->GetVoidPointer(0));

  void* inPtr = input->GetPoints()->GetVoidPointer(0);
  switch (input->GetPoints()->GetDataType())
  {
    svtkTemplateMacro(GenerateNormals<SVTK_TT>::Execute(this, numPts, (SVTK_TT*)inPtr, n,
      this->NormalOrientation, this->OrientationPoint, this->FlipNormals));
  }

  // Orient the normals in a consistent fashion (if requested). This requires a traversal
  // across the point cloud, traversing neighbors that are in close proximity.
  if (this->NormalOrientation == svtkPCANormalEstimation::GRAPH_TRAVERSAL)
  {
    svtkIdType ptId;
    char* pointMap = new char[numPts];
    std::fill_n(pointMap, numPts, static_cast<char>(0));
    svtkIdList* wave = svtkIdList::New();
    wave->Allocate(numPts / 4 + 1, numPts);
    svtkIdList* wave2 = svtkIdList::New();
    wave2->Allocate(numPts / 4 + 1, numPts);

    for (ptId = 0; ptId < numPts; ptId++)
    {
      if (pointMap[ptId] == 0)
      {
        wave->InsertNextId(ptId); // begin next connected wave
        pointMap[ptId] = 1;
        this->TraverseAndFlip(input->GetPoints(), n, pointMap, wave, wave2);
        wave->Reset();
        wave2->Reset();
      }
    } // for all points
    delete[] pointMap;
    wave->Delete();
    wave2->Delete();
  } // if graph traversal required

  // Now send the normals to the output and clean up
  output->SetPoints(input->GetPoints());
  output->GetPointData()->PassData(input->GetPointData());
  output->GetPointData()->SetNormals(normals);
  normals->Delete();

  return 1;
}

//----------------------------------------------------------------------------
// Mark current point as visited and assign cluster number.  Note:
// traversal occurs across proximally located points.
//
void svtkPCANormalEstimation::TraverseAndFlip(
  svtkPoints* inPts, float* normals, char* pointMap, svtkIdList* wave, svtkIdList* wave2)
{
  svtkIdType i, j, numPts, numIds, ptId;
  svtkIdList* tmpWave;
  double x[3];
  float *n, *n2;
  svtkIdList* neighborPointIds = svtkIdList::New();

  while ((numIds = wave->GetNumberOfIds()) > 0)
  {
    for (i = 0; i < numIds; i++) // for all points in this wave
    {
      ptId = wave->GetId(i);
      inPts->GetPoint(ptId, x);
      n = normals + 3 * ptId;
      this->Locator->FindClosestNPoints(this->SampleSize, x, neighborPointIds);

      numPts = neighborPointIds->GetNumberOfIds();
      for (j = 0; j < numPts; ++j)
      {
        ptId = neighborPointIds->GetId(j);
        if (pointMap[ptId] == 0)
        {
          pointMap[ptId] = 1;
          n2 = normals + 3 * ptId;
          if (svtkMath::Dot(n, n2) < 0.0)
          {
            *n2++ *= -1;
            *n2++ *= -1;
            *n2 *= -1;
          }
          wave2->InsertNextId(ptId);
        } // if point not yet visited
      }   // for all neighbors
    }     // for all cells in this wave

    tmpWave = wave;
    wave = wave2;
    wave2 = tmpWave;
    tmpWave->Reset();
  } // while wave is not empty

  neighborPointIds->Delete();
}

//----------------------------------------------------------------------------
int svtkPCANormalEstimation::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkPCANormalEstimation::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sample Size: " << this->SampleSize << "\n";
  os << indent << "Normal Orientation: " << this->NormalOrientation << endl;
  os << indent << "Orientation Point: (" << this->OrientationPoint[0] << ","
     << this->OrientationPoint[1] << "," << this->OrientationPoint[2] << ")\n";
  os << indent << "Flip Normals: " << (this->FlipNormals ? "On\n" : "Off\n");
  os << indent << "Locator: " << this->Locator << "\n";
}
