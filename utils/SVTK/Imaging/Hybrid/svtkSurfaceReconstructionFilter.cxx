/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSurfaceReconstructionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSurfaceReconstructionFilter.h"

#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointLocator.h"
#include "svtkPoints.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkSurfaceReconstructionFilter);

svtkSurfaceReconstructionFilter::svtkSurfaceReconstructionFilter()
{
  this->NeighborhoodSize = 20;
  // negative values cause the algorithm to make a reasonable guess
  this->SampleSpacing = -1.0;
}

// some simple routines for vector math
static void svtkCopyBToA(double* a, double* b)
{
  for (int i = 0; i < 3; i++)
  {
    a[i] = b[i];
  }
}
static void svtkSubtractBFromA(double* a, double* b)
{
  for (int i = 0; i < 3; i++)
  {
    a[i] -= b[i];
  }
}
static void svtkAddBToA(double* a, double* b)
{
  for (int i = 0; i < 3; i++)
  {
    a[i] += b[i];
  }
}
static void svtkMultiplyBy(double* a, double f)
{
  for (int i = 0; i < 3; i++)
  {
    a[i] *= f;
  }
}
static void svtkDivideBy(double* a, double f)
{
  for (int i = 0; i < 3; i++)
  {
    a[i] /= f;
  }
}

// Routines for matrix creation
static void svtkSRFreeMatrix(double** m, long nrl, long nrh, long ncl, long nch);
static double** svtkSRMatrix(long nrl, long nrh, long ncl, long nch);
static void svtkSRFreeVector(double* v, long nl, long nh);
static double* svtkSRVector(long nl, long nh);

// set a matrix to zero
static void svtkSRMakeZero(double** m, long nrl, long nrh, long ncl, long nch)
{
  int i, j;
  for (i = nrl; i <= nrh; i++)
  {
    for (j = ncl; j <= nch; j++)
    {
      m[i][j] = 0.0;
    }
  }
}

// add v*Transpose(v) to m, where v is 3x1 and m is 3x3
void svtkSRAddOuterProduct(double** m, double* v);

// scalar multiply a matrix
static void svtkSRMultiply(double** m, double f, long nrl, long nrh, long ncl, long nch)
{
  int i, j;
  for (i = nrl; i <= nrh; i++)
  {
    for (j = ncl; j <= nch; j++)
    {
      m[i][j] *= f;
    }
  }
}

//----------------------------------------------------------------------------
int svtkSurfaceReconstructionFilter::FillInputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

int svtkSurfaceReconstructionFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // would be nice to compute the whole extent but we need more info to
  // compute it.
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, 1, 0, 1, 0, 1);

  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_FLOAT, 1);
  return 1;
}

//-----------------------------------------------------------------------------
struct SurfacePoint
{
  double loc[3];
  double o[3], n[3];    // plane centre and normal
  svtkIdList* neighbors; // id's of points within LocalRadius of this point
  double* costs;        // should have same length as neighbors, cost for corresponding points
  char isVisited;

  // simple constructor to initialise the members
  SurfacePoint()
    : neighbors(svtkIdList::New())
    , isVisited(0)
  {
  }
  ~SurfacePoint()
  {
    delete[] costs;
    neighbors->Delete();
  }
};

//-----------------------------------------------------------------------------
int svtkSurfaceReconstructionFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the input
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  // get the output
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* output = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  const svtkIdType COUNT = input->GetNumberOfPoints();
  SurfacePoint* surfacePoints;

  svtkIdType i, j;
  int k;

  if (COUNT < 1)
  {
    svtkErrorMacro(<< "No points to reconstruct");
    return 1;
  }
  surfacePoints = new SurfacePoint[COUNT];

  svtkDebugMacro(<< "Reconstructing " << COUNT << " points");

  // time_t start_time,t1,t2,t3,t4;
  // time(&start_time);

  // --------------------------------------------------------------------------
  // 1. Build local connectivity graph
  // -------------------------------------------------------------------------
  {
    svtkPointLocator* locator = svtkPointLocator::New();
    locator->SetDataSet(input);
    svtkIdList* locals = svtkIdList::New();
    // if a pair is close, add each one as a neighbor of the other
    for (i = 0; i < COUNT; i++)
    {
      SurfacePoint* p = &surfacePoints[i];
      svtkCopyBToA(p->loc, input->GetPoint(i));
      locator->FindClosestNPoints(this->NeighborhoodSize, p->loc, locals);
      int iNeighbor;
      for (j = 0; j < locals->GetNumberOfIds(); j++)
      {
        iNeighbor = locals->GetId(j);
        if (iNeighbor != i)
        {
          p->neighbors->InsertNextId(iNeighbor);
          surfacePoints[iNeighbor].neighbors->InsertNextId(i);
        }
      }
    }
    locator->Delete();
    locals->Delete();
  }

  // time(&t1);
  // --------------------------------------------------------------------------
  // 2. Estimate a plane at each point using local points
  // --------------------------------------------------------------------------
  {
    double* pointi;
    double **covar, *v3d, *eigenvalues, **eigenvectors;
    covar = svtkSRMatrix(0, 2, 0, 2);
    v3d = svtkSRVector(0, 2);
    eigenvalues = svtkSRVector(0, 2);
    eigenvectors = svtkSRMatrix(0, 2, 0, 2);
    for (i = 0; i < COUNT; i++)
    {
      SurfacePoint* p = &surfacePoints[i];

      // first find the centroid of the neighbors
      svtkCopyBToA(p->o, p->loc);
      int number = 1;
      svtkIdType neighborIndex;
      for (j = 0; j < p->neighbors->GetNumberOfIds(); j++)
      {
        neighborIndex = p->neighbors->GetId(j);
        pointi = input->GetPoint(neighborIndex);
        svtkAddBToA(p->o, pointi);
        number++;
      }
      svtkDivideBy(p->o, number);
      // then compute the covariance matrix
      svtkSRMakeZero(covar, 0, 2, 0, 2);
      for (k = 0; k < 3; k++)
        v3d[k] = p->loc[k] - p->o[k];
      svtkSRAddOuterProduct(covar, v3d);
      for (j = 0; j < p->neighbors->GetNumberOfIds(); j++)
      {
        neighborIndex = p->neighbors->GetId(j);
        pointi = input->GetPoint(neighborIndex);
        for (k = 0; k < 3; k++)
        {
          v3d[k] = pointi[k] - p->o[k];
        }
        svtkSRAddOuterProduct(covar, v3d);
      }
      svtkSRMultiply(covar, 1.0 / number, 0, 2, 0, 2);
      // then extract the third eigenvector
      svtkMath::Jacobi(covar, eigenvalues, eigenvectors);
      // third eigenvector (column 2, ordered by eigenvalue magnitude) is plane normal
      for (k = 0; k < 3; k++)
      {
        p->n[k] = eigenvectors[k][2];
      }
    }
    svtkSRFreeMatrix(covar, 0, 2, 0, 2);
    svtkSRFreeVector(v3d, 0, 2);
    svtkSRFreeVector(eigenvalues, 0, 2);
    svtkSRFreeMatrix(eigenvectors, 0, 2, 0, 2);
  }

  // time(&t2);
  //--------------------------------------------------------------------------
  // 3a. Compute a cost between every pair of neighbors for the MST
  // --------------------------------------------------------------------------
  // cost = 1 - |normal1.normal2|
  // ie. cost is 0 if planes are parallel, 1 if orthogonal (least parallel)
  for (i = 0; i < COUNT; i++)
  {
    SurfacePoint* p = &surfacePoints[i];
    p->costs = new double[p->neighbors->GetNumberOfIds()];

    // compute cost between all its neighbors
    // (bit inefficient to do this for every point, as cost is symmetric)
    for (j = 0; j < p->neighbors->GetNumberOfIds(); j++)
    {
      p->costs[j] = 1.0 - fabs(svtkMath::Dot(p->n, surfacePoints[p->neighbors->GetId(j)].n));
    }
  }

  // --------------------------------------------------------------------------
  // 3b. Ensure consistency in plane direction between neighbors
  // --------------------------------------------------------------------------
  // method: guess first one, then walk through tree along most-parallel
  // neighbors MST, flipping the new normal if inconsistent

  // to walk minimal spanning tree, keep record of vertices visited and list
  // of those near to any visited point but not themselves visited. Start
  // with just one vertex as visited.  Pick the vertex in the neighbors list
  // that has the lowest cost connection with a visited vertex. Record this
  // vertex as visited, add any new neighbors to the neighbors list.

  // Disable if you don't want orientation propagation (for testing)
#if 1
  {
    svtkIdList* nearby = svtkIdList::New(); // list of nearby, unvisited points

    // start with some vertex
    int first = 0; // index of starting vertex
    surfacePoints[first].isVisited = 1;
    // add all the neighbors of the starting vertex into nearby
    for (j = 0; j < surfacePoints[first].neighbors->GetNumberOfIds(); j++)
    {
      nearby->InsertNextId(surfacePoints[first].neighbors->GetId(j));
    }

    double cost, lowestCost;
    int cheapestNearby = 0, connectedVisited = 0;

    // repeat until nearby is empty:
    while (nearby->GetNumberOfIds() > 0)
    {
      // for each nearby point:
      svtkIdType iNearby, iNeighbor;
      lowestCost = SVTK_DOUBLE_MAX;
      for (i = 0; i < nearby->GetNumberOfIds(); i++)
      {
        iNearby = nearby->GetId(i);
        // test cost against all neighbors that are members of visited
        for (j = 0; j < surfacePoints[iNearby].neighbors->GetNumberOfIds(); j++)
        {
          iNeighbor = surfacePoints[iNearby].neighbors->GetId(j);
          if (surfacePoints[iNeighbor].isVisited)
          {
            cost = surfacePoints[iNearby].costs[j];
            // pick lowest cost for this nearby point
            if (cost < lowestCost)
            {
              lowestCost = cost;
              cheapestNearby = iNearby;
              connectedVisited = iNeighbor;
              // optional: can break out if satisfied with parallelness
              if (lowestCost < 0.1)
              {
                i = nearby->GetNumberOfIds();
                break;
              }
            }
          }
        }
      }
      if (connectedVisited == cheapestNearby)
      {
        svtkErrorMacro(<< "Internal error in svtkSurfaceReconstructionFilter");
        return 0;
      }

      // correct the orientation of the point if necessary
      if (svtkMath::Dot(surfacePoints[cheapestNearby].n, surfacePoints[connectedVisited].n) < 0.0F)
      {
        // flip this normal
        svtkMultiplyBy(surfacePoints[cheapestNearby].n, -1);
      }
      // add this nearby point to visited
      if (surfacePoints[cheapestNearby].isVisited != 0)
      {
        svtkErrorMacro(<< "Internal error in svtkSurfaceReconstructionFilter");
        return 0;
      }

      surfacePoints[cheapestNearby].isVisited = 1;
      // remove from nearby
      nearby->DeleteId(cheapestNearby);
      // add all new nearby points to nearby
      for (j = 0; j < surfacePoints[cheapestNearby].neighbors->GetNumberOfIds(); j++)
      {
        iNeighbor = surfacePoints[cheapestNearby].neighbors->GetId(j);
        if (surfacePoints[iNeighbor].isVisited == 0)
        {
          nearby->InsertUniqueId(iNeighbor);
        }
      }
    }

    nearby->Delete();
  }
#endif

  // time(&t3);

  // --------------------------------------------------------------------------
  // 4. Compute signed distance to surface for every point on a 3D grid
  // --------------------------------------------------------------------------
  {
    // need to know the bounding rectangle
    double bounds[6];
    for (i = 0; i < 3; i++)
    {
      bounds[i * 2] = input->GetBounds()[i * 2];
      bounds[i * 2 + 1] = input->GetBounds()[i * 2 + 1];
    }

    // estimate the spacing if required
    if (this->SampleSpacing <= 0.0)
    {
      // spacing guessed as cube root of (volume divided by number of points)
      this->SampleSpacing = pow(static_cast<double>(bounds[1] - bounds[0]) *
          (bounds[3] - bounds[2]) * (bounds[5] - bounds[4]) / static_cast<double>(COUNT),
        static_cast<double>(1.0 / 3.0));

      svtkDebugMacro(<< "Estimated sample spacing as: " << this->SampleSpacing);
    }

    // allow a border around the volume to allow sampling around the extremes
    for (i = 0; i < 3; i++)
    {
      bounds[i * 2] -= this->SampleSpacing * 2;
      bounds[i * 2 + 1] += this->SampleSpacing * 2;
    }

    double topleft[3] = { bounds[0], bounds[2], bounds[4] };
    double bottomright[3] = { bounds[1], bounds[3], bounds[5] };
    int dim[3];
    for (i = 0; i < 3; i++)
    {
      dim[i] = static_cast<int>((bottomright[i] - topleft[i]) / this->SampleSpacing);
    }

    svtkDebugMacro(<< "Created output volume of dimensions: (" << dim[0] << ", " << dim[1] << ", "
                  << dim[2] << ")");

    // initialise the output volume
    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, dim[0] - 1, 0, dim[1] - 1, 0,
      dim[2] - 1);
    output->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);
    output->SetOrigin(bounds[0], bounds[2],
      bounds[4]); // these bounds take into account the extra border space introduced above
    output->SetSpacing(this->SampleSpacing, this->SampleSpacing, this->SampleSpacing);
    output->AllocateScalars(outInfo);
    outInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), 0, dim[0] - 1, 0, dim[1] - 1, 0,
      dim[2] - 1);

    svtkFloatArray* newScalars =
      svtkArrayDownCast<svtkFloatArray>(output->GetPointData()->GetScalars());
    outInfo->Set(
      svtkDataObject::SPACING(), this->SampleSpacing, this->SampleSpacing, this->SampleSpacing);
    outInfo->Set(svtkDataObject::ORIGIN(), topleft, 3);

    // initialise the point locator (have to use point insertion because we
    // need to set our own bounds, slightly larger than the dataset to allow
    // for sampling around the edge)
    svtkPointLocator* locator = svtkPointLocator::New();
    svtkPoints* newPts = svtkPoints::New();
    locator->InitPointInsertion(newPts, bounds, static_cast<int>(COUNT));
    for (i = 0; i < COUNT; i++)
    {
      locator->InsertPoint(i, surfacePoints[i].loc);
    }

    // go through the array probing the values
    int x, y, z;
    int iClosestPoint;
    int zOffset, yOffset, offset;
    double probeValue;
    double point[3], temp[3];
    for (z = 0; z < dim[2]; z++)
    {
      zOffset = z * dim[1] * dim[0];
      point[2] = topleft[2] + z * this->SampleSpacing;
      for (y = 0; y < dim[1]; y++)
      {
        yOffset = y * dim[0] + zOffset;
        point[1] = topleft[1] + y * this->SampleSpacing;
        for (x = 0; x < dim[0]; x++)
        {
          offset = x + yOffset;
          point[0] = topleft[0] + x * this->SampleSpacing;
          // find the distance from the probe to the plane of the nearest point
          iClosestPoint = locator->FindClosestInsertedPoint(point);
          if (iClosestPoint == -1)
          {
            svtkErrorMacro(<< "Internal error");
            return 0;
          }
          svtkCopyBToA(temp, point);
          svtkSubtractBFromA(temp, surfacePoints[iClosestPoint].loc);
          probeValue = svtkMath::Dot(temp, surfacePoints[iClosestPoint].n);
          newScalars->SetValue(offset, probeValue);
        }
      }
    }
    locator->Delete();
    newPts->Delete();
  }

  // time(&t4);
  // Clear up everything
  delete[] surfacePoints;

  return 1;
}

void svtkSurfaceReconstructionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Neighborhood Size:" << this->NeighborhoodSize << "\n";
  os << indent << "Sample Spacing:" << this->SampleSpacing << "\n";
}

void svtkSRAddOuterProduct(double** m, double* v)
{
  int i, j;
  for (i = 0; i < 3; i++)
  {
    for (j = 0; j < 3; j++)
    {
      m[i][j] += v[i] * v[j];
    }
  }
}

#define SVTK_NR_END 1

// allocate a float vector with subscript range v[nl..nh]
// must be balanced with svtkSRFreeVector.
static double* svtkSRVector(long nl, long nh)
{
  double* v = new double[nh - nl + 1 + SVTK_NR_END];
  if (!v)
  {
    svtkGenericWarningMacro(<< "allocation failure in vector()");
    return nullptr;
  }

  return (v - nl + SVTK_NR_END);
}

// allocate a float matrix with subscript range m[nrl..nrh][ncl..nch]
// must be balanced with svtkSRFreeMatrix.
static double** svtkSRMatrix(long nrl, long nrh, long ncl, long nch)
{
  long i, nrow = nrh - nrl + 1, ncol = nch - ncl + 1;
  double** m;

  // allocate pointers to rows
  m = new double*[nrow + SVTK_NR_END];
  if (!m)
  {
    svtkGenericWarningMacro(<< "allocation failure 1 in Matrix()");
    return nullptr;
  }

  m += SVTK_NR_END;
  m -= nrl;

  // allocate rows and set pointers to them
  m[nrl] = new double[nrow * ncol + SVTK_NR_END];
  if (!m[nrl])
  {
    svtkGenericWarningMacro("allocation failure 2 in Matrix()");
    return nullptr;
  }

  m[nrl] += SVTK_NR_END;
  m[nrl] -= ncl;
  for (i = nrl + 1; i <= nrh; i++)
  {
    m[i] = m[i - 1] + ncol;
  }

  // return pointer to array of pointers to rows
  return m;
}

// free a double vector allocated with SRVector()
static void svtkSRFreeVector(double* v, long nl, long svtkNotUsed(nh))
{
  delete[](v + nl - SVTK_NR_END);
}

// free a double matrix allocated by Matrix()
static void svtkSRFreeMatrix(
  double** m, long nrl, long svtkNotUsed(nrh), long ncl, long svtkNotUsed(nch))

{
  delete[](m[nrl] + ncl - SVTK_NR_END);
  delete[](m + nrl - SVTK_NR_END);
}

#undef SVTK_NR_END
