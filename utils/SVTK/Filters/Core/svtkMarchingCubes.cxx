/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMarchingCubes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMarchingCubes.h"

#include "svtkArrayDispatch.h"
#include "svtkCellArray.h"
#include "svtkCharArray.h"
#include "svtkDataArrayRange.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkImageTransform.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkMarchingCubesTriangleCases.h"
#include "svtkMath.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkShortArray.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredPoints.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedShortArray.h"

svtkStandardNewMacro(svtkMarchingCubes);

// Description:
// Construct object with initial range (0,1) and single contour value
// of 0.0. ComputeNormal is on, ComputeGradients is off and ComputeScalars is on.
svtkMarchingCubes::svtkMarchingCubes()
{
  this->ContourValues = svtkContourValues::New();
  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;
  this->Locator = nullptr;
}

svtkMarchingCubes::~svtkMarchingCubes()
{
  this->ContourValues->Delete();
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
}

// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
svtkMTimeType svtkMarchingCubes::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType mTime2 = this->ContourValues->GetMTime();

  mTime = (mTime2 > mTime ? mTime2 : mTime);
  if (this->Locator)
  {
    mTime2 = this->Locator->GetMTime();
    mTime = (mTime2 > mTime ? mTime2 : mTime);
  }

  return mTime;
}

namespace {

// Calculate the gradient using central difference.
// NOTE: We calculate the negative of the gradient for efficiency
template <class ScalarRangeT>
void svtkMarchingCubesComputePointGradient(int i, int j, int k,
                                          const ScalarRangeT s, int dims[3],
                                          svtkIdType sliceSize, double n[3])
{
  double sp, sm;

  // x-direction
  if (i == 0)
  {
    sp = s[i + 1 + j * dims[0] + k * sliceSize];
    sm = s[i + j * dims[0] + k * sliceSize];
    n[0] = sm - sp;
  }
  else if (i == (dims[0] - 1))
  {
    sp = s[i + j * dims[0] + k * sliceSize];
    sm = s[i - 1 + j * dims[0] + k * sliceSize];
    n[0] = sm - sp;
  }
  else
  {
    sp = s[i + 1 + j * dims[0] + k * sliceSize];
    sm = s[i - 1 + j * dims[0] + k * sliceSize];
    n[0] = 0.5 * (sm - sp);
  }

  // y-direction
  if (j == 0)
  {
    sp = s[i + (j + 1) * dims[0] + k * sliceSize];
    sm = s[i + j * dims[0] + k * sliceSize];
    n[1] = sm - sp;
  }
  else if (j == (dims[1] - 1))
  {
    sp = s[i + j * dims[0] + k * sliceSize];
    sm = s[i + (j - 1) * dims[0] + k * sliceSize];
    n[1] = sm - sp;
  }
  else
  {
    sp = s[i + (j + 1) * dims[0] + k * sliceSize];
    sm = s[i + (j - 1) * dims[0] + k * sliceSize];
    n[1] = 0.5 * (sm - sp);
  }

  // z-direction
  if (k == 0)
  {
    sp = s[i + j * dims[0] + (k + 1) * sliceSize];
    sm = s[i + j * dims[0] + k * sliceSize];
    n[2] = sm - sp;
  }
  else if (k == (dims[2] - 1))
  {
    sp = s[i + j * dims[0] + k * sliceSize];
    sm = s[i + j * dims[0] + (k - 1) * sliceSize];
    n[2] = sm - sp;
  }
  else
  {
    sp = s[i + j * dims[0] + (k + 1) * sliceSize];
    sm = s[i + j * dims[0] + (k - 1) * sliceSize];
    n[2] = 0.5 * (sm - sp);
  }
}

//
// Contouring filter specialized for volumes and "short int" data values.
//
struct ComputeGradientWorker
{
  template <class ScalarArrayT>
  void operator()(ScalarArrayT *scalarsArray,
                  svtkMarchingCubes *self,
                  int dims[3],
                  svtkIncrementalPointLocator *locator,
                  svtkDataArray *newScalars,
                  svtkDataArray *newGradients,
                  svtkDataArray *newNormals,
                  svtkCellArray *newPolys,
                  double *values,
                  svtkIdType numValues) const
  {
    const auto scalars = svtk::DataArrayValueRange<1>(scalarsArray);

    double s[8], value;
    int i, j, k;
    svtkIdType sliceSize;
    static const int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
    svtkMarchingCubesTriangleCases *triCase, *triCases;
    EDGE_LIST  *edge;
    int contNum, jOffset, ii, index, *vert;
    svtkIdType kOffset, idx;
    svtkIdType ptIds[3];
    int ComputeNormals = newNormals != nullptr;
    int ComputeGradients = newGradients != nullptr;
    int ComputeScalars = newScalars != nullptr;
    int NeedGradients;
    int extent[6];
    double t, *x1, *x2, x[3], *n1, *n2, n[3], min, max;
    double pts[8][3], gradients[8][3], xp, yp, zp;
    static int edges[12][2] = { {0,1}, {1,2}, {3,2}, {0,3},
                                {4,5}, {5,6}, {7,6}, {4,7},
                                {0,4}, {1,5}, {3,7}, {2,6}};

    svtkInformation *inInfo = self->GetExecutive()->GetInputInformation(0, 0);
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),extent);

    triCases =  svtkMarchingCubesTriangleCases::GetCases();

    //
    // Get min/max contour values
    //
    if ( numValues < 1 )
    {
      return;
    }

    for ( min=max=values[0], i=1; i < numValues; i++)
    {
      if ( values[i] < min )
      {
        min = values[i];
      }
      if ( values[i] > max )
      {
        max = values[i];
      }
    }
    //
    // Traverse all voxel cells, generating triangles and point gradients
    // using marching cubes algorithm.
    //
    sliceSize = dims[0] * dims[1];
    for ( k=0; k < (dims[2]-1); k++)
    {
      self->UpdateProgress (k / static_cast<double>(dims[2] - 1));
      if (self->GetAbortExecute())
      {
        break;
      }
      kOffset = k*sliceSize;
      pts[0][2] = k+extent[4];
      zp = pts[0][2] + 1;
      for ( j=0; j < (dims[1]-1); j++)
      {
        jOffset = j*dims[0];
        pts[0][1] = j+extent[2];
        yp = pts[0][1] + 1;
        for ( i=0; i < (dims[0]-1); i++)
        {
          //get scalar values
          idx = i + jOffset + kOffset;
          s[0] = scalars[idx];
          s[1] = scalars[idx+1];
          s[2] = scalars[idx+1 + dims[0]];
          s[3] = scalars[idx + dims[0]];
          s[4] = scalars[idx + sliceSize];
          s[5] = scalars[idx+1 + sliceSize];
          s[6] = scalars[idx+1 + dims[0] + sliceSize];
          s[7] = scalars[idx + dims[0] + sliceSize];

          if ( (s[0] < min && s[1] < min && s[2] < min && s[3] < min &&
                s[4] < min && s[5] < min && s[6] < min && s[7] < min) ||
               (s[0] > max && s[1] > max && s[2] > max && s[3] > max &&
                s[4] > max && s[5] > max && s[6] > max && s[7] > max) )
          {
            continue; // no contours possible
          }

          //create voxel points
          pts[0][0] = i+extent[0];
          xp = pts[0][0] + 1;

          pts[1][0] = xp;
          pts[1][1] = pts[0][1];
          pts[1][2] = pts[0][2];

          pts[2][0] = xp;
          pts[2][1] = yp;
          pts[2][2] = pts[0][2];

          pts[3][0] = pts[0][0];
          pts[3][1] = yp;
          pts[3][2] = pts[0][2];

          pts[4][0] = pts[0][0];
          pts[4][1] = pts[0][1];
          pts[4][2] = zp;

          pts[5][0] = xp;
          pts[5][1] = pts[0][1];
          pts[5][2] = zp;

          pts[6][0] = xp;
          pts[6][1] = yp;
          pts[6][2] = zp;

          pts[7][0] = pts[0][0];
          pts[7][1] = yp;
          pts[7][2] = zp;

          NeedGradients = ComputeGradients || ComputeNormals;

          //create gradients if needed
          if (NeedGradients)
          {
            svtkMarchingCubesComputePointGradient(i,j,k, scalars, dims, sliceSize, gradients[0]);
            svtkMarchingCubesComputePointGradient(i+1,j,k, scalars, dims, sliceSize, gradients[1]);
            svtkMarchingCubesComputePointGradient(i+1,j+1,k, scalars, dims, sliceSize, gradients[2]);
            svtkMarchingCubesComputePointGradient(i,j+1,k, scalars, dims, sliceSize, gradients[3]);
            svtkMarchingCubesComputePointGradient(i,j,k+1, scalars, dims, sliceSize, gradients[4]);
            svtkMarchingCubesComputePointGradient(i+1,j,k+1, scalars, dims, sliceSize, gradients[5]);
            svtkMarchingCubesComputePointGradient(i+1,j+1,k+1, scalars, dims, sliceSize, gradients[6]);
            svtkMarchingCubesComputePointGradient(i,j+1,k+1, scalars, dims, sliceSize, gradients[7]);
          }
          for (contNum=0; contNum < numValues; contNum++)
          {
            value = values[contNum];
            // Build the case table
            for ( ii=0, index = 0; ii < 8; ii++)
            {
              if ( s[ii] >= value )
              {
                index |= CASE_MASK[ii];
              }
            }
            if ( index == 0 || index == 255 ) //no surface
            {
              continue;
            }
            triCase = triCases+ index;
            edge = triCase->edges;

            for ( ; edge[0] > -1; edge += 3 )
            {
              for (ii=0; ii<3; ii++) //insert triangle
              {
                vert = edges[edge[ii]];
                t = (value - s[vert[0]]) / (s[vert[1]] - s[vert[0]]);
                x1 = pts[vert[0]];
                x2 = pts[vert[1]];
                x[0] = x1[0] + t * (x2[0] - x1[0]);
                x[1] = x1[1] + t * (x2[1] - x1[1]);
                x[2] = x1[2] + t * (x2[2] - x1[2]);

                // check for a new point
                if ( locator->InsertUniquePoint(x, ptIds[ii]) )
                {
                  if (NeedGradients)
                  {
                    n1 = gradients[vert[0]];
                    n2 = gradients[vert[1]];
                    n[0] = n1[0] + t * (n2[0] - n1[0]);
                    n[1] = n1[1] + t * (n2[1] - n1[1]);
                    n[2] = n1[2] + t * (n2[2] - n1[2]);
                  }
                  if (ComputeScalars)
                  {
                    newScalars->InsertTuple(ptIds[ii],&value);
                  }
                  if (ComputeGradients)
                  {
                    newGradients->InsertTuple(ptIds[ii],n);
                  }
                  if (ComputeNormals)
                  {
                    svtkMath::Normalize(n);
                    newNormals->InsertTuple(ptIds[ii],n);
                  }
                }
              }
              // check for degenerate triangle
              if ( ptIds[0] != ptIds[1] &&
                   ptIds[0] != ptIds[2] &&
                   ptIds[1] != ptIds[2] )
              {
                newPolys->InsertNextCell(3,ptIds);
              }
            }//for each triangle
          }//for all contours
        }//for i
      }//for j
    }//for k
  }
};

} // end anon namespace

//
// Contouring filter specialized for volumes and "short int" data values.
//
int svtkMarchingCubes::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkImageData* input = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPoints* newPts;
  svtkCellArray* newPolys;
  svtkFloatArray* newScalars;
  svtkFloatArray* newNormals;
  svtkFloatArray* newGradients;
  svtkPointData* pd;
  svtkDataArray* inScalars;
  int dims[3], extent[6];
  svtkIdType estimatedSize;
  double bounds[6];
  svtkIdType numContours = this->ContourValues->GetNumberOfContours();
  double* values = this->ContourValues->GetValues();

  svtkDebugMacro(<< "Executing marching cubes");

  //
  // Initialize and check input
  //
  pd = input->GetPointData();
  if (pd == nullptr)
  {
    svtkErrorMacro(<< "PointData is nullptr");
    return 1;
  }
  svtkInformationVector* inArrayVec = this->Information->Get(INPUT_ARRAYS_TO_PROCESS());
  if (inArrayVec)
  { // we have been passed an input array
    inScalars = this->GetInputArrayToProcess(0, inputVector);
  }
  else
  {
    inScalars = pd->GetScalars();
  }
  if (inScalars == nullptr)
  {
    svtkErrorMacro(<< "Scalars must be defined for contouring");
    return 1;
  }

  if (inScalars->GetNumberOfComponents() != 1)
  {
    svtkErrorMacro("Scalar array must only have a single component.");
    return 1;
  }

  if ( input->GetDataDimension() != 3 )
  {
    svtkErrorMacro(<< "Cannot contour data of dimension != 3");
    return 1;
  }
  input->GetDimensions(dims);

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);

  // estimate the number of points from the volume dimensions
  estimatedSize = static_cast<svtkIdType>(pow(1.0 * dims[0] * dims[1] * dims[2], 0.75));
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }
  svtkDebugMacro(<< "Estimated allocation size is " << estimatedSize);
  newPts = svtkPoints::New();
  newPts->Allocate(estimatedSize, estimatedSize / 2);
  // compute bounds for merging points
  for (int i = 0; i < 3; i++)
  {
    bounds[2 * i] = extent[2 * i];
    bounds[2 * i + 1] = extent[2 * i + 1];
  }
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPts, bounds, estimatedSize);

  if (this->ComputeNormals)
  {
    newNormals = svtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->Allocate(3 * estimatedSize, 3 * estimatedSize / 2);
  }
  else
  {
    newNormals = nullptr;
  }

  if (this->ComputeGradients)
  {
    newGradients = svtkFloatArray::New();
    newGradients->SetNumberOfComponents(3);
    newGradients->Allocate(3 * estimatedSize, 3 * estimatedSize / 2);
  }
  else
  {
    newGradients = nullptr;
  }

  newPolys = svtkCellArray::New();
  newPolys->AllocateEstimate(estimatedSize, 3);

  if (this->ComputeScalars)
  {
    newScalars = svtkFloatArray::New();
    newScalars->Allocate(estimatedSize, estimatedSize / 2);
  }
  else
  {
    newScalars = nullptr;
  }

  using Dispatcher = svtkArrayDispatch::Dispatch;
  ComputeGradientWorker worker;
  if (!Dispatcher::Execute(inScalars, worker, this, dims, this->Locator,
                           newScalars, newGradients, newNormals, newPolys,
                           values, numContours))
  { // Fallback to slow path for unknown arrays:
    worker(inScalars, this, dims, this->Locator, newScalars, newGradients,
           newNormals, newPolys, values, numContours);
  }

  svtkDebugMacro(<< "Created: " << newPts->GetNumberOfPoints() << " points, "
                << newPolys->GetNumberOfCells() << " triangles");
  //
  // Update ourselves.  Because we don't know up front how many triangles
  // we've created, take care to reclaim memory.
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  if (newScalars)
  {
    int idx = output->GetPointData()->AddArray(newScalars);
    output->GetPointData()->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }
  if (newGradients)
  {
    output->GetPointData()->SetVectors(newGradients);
    newGradients->Delete();
  }
  if (newNormals)
  {
    output->GetPointData()->SetNormals(newNormals);
    newNormals->Delete();
  }
  output->Squeeze();
  if (this->Locator)
  {
    this->Locator->Initialize(); // free storage
  }

  svtkImageTransform::TransformPointSet(input, output);

  return 1;
}

// Description:
// Specify a spatial locator for merging points. By default,
// an instance of svtkMergePoints is used.
void svtkMarchingCubes::SetLocator(svtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }

  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }

  if (locator)
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

void svtkMarchingCubes::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
  }
}

int svtkMarchingCubes::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

void svtkMarchingCubes::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");

  if (this->Locator)
  {
    os << indent << "Locator:" << this->Locator << "\n";
    this->Locator->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }
}
