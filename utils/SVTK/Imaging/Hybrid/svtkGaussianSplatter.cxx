/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGaussianSplatter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGaussianSplatter.h"

#include "svtkCompositeDataIterator.h"
#include "svtkCompositeDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSMPTools.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <cmath>

svtkStandardNewMacro(svtkGaussianSplatter);

//----------------------------------------------------------------------------
// Algorithm and integration into svtkSMPTools
class svtkGaussianSplatterAlgorithm
{
public:
  svtkGaussianSplatter* Splatter;
  double* Scalars;
  svtkIdType Dims[3], SliceSize;
  double Origin[3], Spacing[3], Radius2;

  class Splat
  {
  public:
    svtkGaussianSplatterAlgorithm* Algo;
    svtkIdType XMin, XMax, YMin, YMax, ZMin, ZMax;
    Splat(svtkGaussianSplatterAlgorithm* algo) { this->Algo = algo; }
    void SetBounds(int min[3], int max[3])
    {
      this->XMin = min[0];
      this->XMax = max[0];
      this->YMin = min[1];
      this->YMax = max[1];
      this->ZMin = min[2];
      this->ZMax = max[2];
    }
    void operator()(svtkIdType slice, svtkIdType end)
    {
      svtkIdType i, j, jOffset, kOffset, idx;
      double cx[3], dist2;
      for (; slice < end; ++slice)
      {
        // Loop over all sample points in volume within footprint and
        // evaluate the splat
        cx[2] = this->Algo->Origin[2] + this->Algo->Spacing[2] * slice;
        kOffset = slice * this->Algo->SliceSize;
        for (j = this->YMin; j <= this->YMax; j++)
        {
          cx[1] = this->Algo->Origin[1] + this->Algo->Spacing[1] * j;
          jOffset = j * this->Algo->Dims[0];
          for (i = this->XMin; i <= this->XMax; i++)
          {
            cx[0] = this->Algo->Origin[0] + this->Algo->Spacing[0] * i;
            dist2 = this->Algo->Splatter->SamplePoint(cx);
            if (dist2 <= this->Algo->Radius2)
            {
              idx = i + jOffset + kOffset;
              this->Algo->Splatter->SetScalar(idx, dist2, this->Algo->Scalars + idx);
            } // if within splat radius
          }   // i
        }     // j
      }       // k within splat footprint
    }
  };
};

//----------------------------------------------------------------------------
// Create the SVTK class proper.  Construct object with dimensions=(50,50,50);
// automatic computation of bounds; a splat radius of 0.1; an exponent factor
// of -5; and normal and scalar warping turned on.
svtkGaussianSplatter::svtkGaussianSplatter()
{
  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->Radius = 0.1;
  this->ExponentFactor = -5.0;

  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->NormalWarping = 1;
  this->Eccentricity = 2.5;

  this->ScalarWarping = 1;
  this->ScaleFactor = 1.0;

  this->Capping = 1;
  this->CapValue = 0.0;

  this->AccumulationMode = SVTK_ACCUMULATION_MODE_MAX;
  this->NullValue = 0.0;
}

//----------------------------------------------------------------------------
int svtkGaussianSplatter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // use model bounds if set
  this->Origin[0] = 0;
  this->Origin[1] = 0;
  this->Origin[2] = 0;
  if (this->ModelBounds[0] < this->ModelBounds[1] && this->ModelBounds[2] < this->ModelBounds[3] &&
    this->ModelBounds[4] < this->ModelBounds[5])
  {
    this->Origin[0] = this->ModelBounds[0];
    this->Origin[1] = this->ModelBounds[2];
    this->Origin[2] = this->ModelBounds[4];
  }

  outInfo->Set(svtkDataObject::ORIGIN(), this->Origin, 3);

  int i;
  for (i = 0; i < 3; i++)
  {
    this->Spacing[i] =
      (this->ModelBounds[2 * i + 1] - this->ModelBounds[2 * i]) / (this->SampleDimensions[i] - 1);
    if (this->Spacing[i] <= 0.0)
    {
      this->Spacing[i] = 1.0;
    }
  }
  outInfo->Set(svtkDataObject::SPACING(), this->Spacing, 3);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, this->SampleDimensions[0] - 1,
    0, this->SampleDimensions[1] - 1, 0, this->SampleDimensions[2] - 1);
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_DOUBLE, 1);
  return 1;
}

//----------------------------------------------------------------------------
int svtkGaussianSplatter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the data object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* output = svtkImageData::GetData(outputVector, 0);

  output->SetExtent(outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  output->AllocateScalars(outInfo);

  svtkIdType totalNumPts, numNewPts, ptId, i;
  int min[3], max[3];
  svtkPointData* pd;
  svtkDataArray* inNormals = nullptr;
  double loc[3];
  svtkDoubleArray* newScalars =
    svtkArrayDownCast<svtkDoubleArray>(output->GetPointData()->GetScalars());
  newScalars->SetName("SplatterValues");

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataSet* inputDS = svtkDataSet::GetData(inInfo);
  svtkCompositeDataSet* inputComposite = svtkCompositeDataSet::GetData(inInfo);
  svtkNew<svtkMultiBlockDataSet> tempComposite;
  if (inputComposite == nullptr)
  {
    tempComposite->SetNumberOfBlocks(1);
    tempComposite->SetBlock(0, inputDS);
    inputComposite = tempComposite;
  }

  svtkDebugMacro(<< "Splatting data");

  //  Make sure points are available
  //
  totalNumPts = inputComposite->GetNumberOfPoints();
  if (totalNumPts == 0)
  {
    svtkDebugMacro(<< "No points to splat!");
    svtkWarningMacro(<< "No POINTS!!");
    return 1;
  }

  svtkSmartPointer<svtkCompositeDataIterator> dataItr =
    svtkSmartPointer<svtkCompositeDataIterator>::Take(inputComposite->NewIterator());

  // decide which array to splat, if any
  dataItr->InitTraversal();
  svtkDataSet* ds = nullptr;
  while (ds == nullptr && !dataItr->IsDoneWithTraversal())
  {
    ds = svtkDataSet::SafeDownCast(dataItr->GetCurrentDataObject());
  }
  if (ds == nullptr)
  {
    svtkDebugMacro(<< "The input is an empty block structure");
    return 1;
  }

  output->SetDimensions(this->GetSampleDimensions());
  this->ComputeModelBounds(inputComposite, output, outInfo);

  //  Compute the radius of influence of the points.  If an
  //  automatically generated bounding box has been generated, increase
  //  its size slightly to acoomodate the radius of influence.
  //
  this->Eccentricity2 = this->Eccentricity * this->Eccentricity;

  numNewPts = this->SampleDimensions[0] * this->SampleDimensions[1] * this->SampleDimensions[2];
  double* scalars = newScalars->WritePointer(0, numNewPts);
  std::fill_n(scalars, numNewPts, this->NullValue);
  this->Visited = new char[numNewPts];
  std::fill_n(this->Visited, numNewPts, 0);

  pd = ds->GetPointData();
  bool useScalars = false;
  int association = svtkDataObject::FIELD_ASSOCIATION_POINTS;
  svtkDataArray* inScalars = this->GetInputArrayToProcess(0, ds, association);
  if (!inScalars)
  {
    inScalars = pd->GetScalars();
    useScalars = true;
  }

  //  Set up function pointers to sample functions
  //
  if (this->NormalWarping && (inNormals = pd->GetNormals()) != nullptr)
  {
    this->Sample = &svtkGaussianSplatter::EccentricGaussian;
  }
  else
  {
    this->Sample = &svtkGaussianSplatter::Gaussian;
  }

  if (this->ScalarWarping && inScalars != nullptr)
  {
    this->SampleFactor = &svtkGaussianSplatter::ScalarSampling;
  }
  else
  {
    this->SampleFactor = &svtkGaussianSplatter::PositionSampling;
    this->S = 0.0; // position sampling does not require S to be defined
                   // but this makes purify happy.
  }

  // Prepare for parallel splatting
  svtkGaussianSplatterAlgorithm algo;
  algo.Splatter = this;
  algo.Scalars = scalars;
  algo.Radius2 = this->Radius2;
  algo.SliceSize = this->SampleDimensions[0] * this->SampleDimensions[1];
  for (i = 0; i < 3; ++i)
  {
    algo.Dims[i] = this->SampleDimensions[i];
    algo.Origin[i] = this->Origin[i];
    algo.Spacing[i] = this->Spacing[i];
  }
  svtkGaussianSplatterAlgorithm::Splat splat(&algo);

  // Process all input datasets
  for (dataItr->InitTraversal(); !dataItr->IsDoneWithTraversal(); dataItr->GoToNextItem())
  {
    svtkDataSet* input = svtkDataSet::SafeDownCast(dataItr->GetCurrentDataObject());
    if (!input)
    {
      continue;
    }
    svtkDataArray* myScalars = nullptr;
    if (inScalars != nullptr)
    {
      if (useScalars)
      {
        myScalars = input->GetPointData()->GetScalars();
      }
      else
      {
        myScalars = this->GetInputArrayToProcess(0, input, association);
      }
    }
    if (inScalars != nullptr && myScalars == nullptr)
    {
      svtkWarningMacro(<< "Piece does not have selected scalars array");
      continue;
    }
    svtkDataArray* myNormals = nullptr;
    if (inNormals != nullptr)
    {
      myNormals = input->GetPointData()->GetNormals();
    }
    if (this->NormalWarping && inNormals != nullptr && myNormals == nullptr)
    {
      svtkWarningMacro(<< "Piece does not have required normals array");
      continue;
    }
    svtkIdType numPts = input->GetNumberOfPoints();

    // Traverse all points - splatting each into the volume.
    // For each point, determine which voxel it is in.  Then determine
    // the subvolume that the splat is contained in, and process that.
    //
    int abortExecute = 0;
    svtkIdType progressInterval = numPts / 20 + 1;
    for (ptId = 0; ptId < numPts && !abortExecute; ptId++)
    {
      if (!(ptId % progressInterval))
      {
        svtkDebugMacro(<< "Inserting point #" << ptId);
        this->UpdateProgress(static_cast<double>(ptId) / numPts);
        abortExecute = this->GetAbortExecute();
      }

      this->P = input->GetPoint(ptId);
      if (myNormals != nullptr)
      {
        this->N = myNormals->GetTuple(ptId);
      }
      if (myScalars != nullptr)
      {
        this->S = myScalars->GetComponent(ptId, 0);
      }

      // Determine the voxel that the point is in
      for (i = 0; i < 3; i++)
      {
        loc[i] = (this->P[i] - this->Origin[i]) / this->Spacing[i];
      }

      // Determine splat footprint
      for (i = 0; i < 3; i++)
      {
        min[i] = static_cast<int>(floor(static_cast<double>(loc[i]) - this->SplatDistance[i]));
        max[i] = static_cast<int>(ceil(static_cast<double>(loc[i]) + this->SplatDistance[i]));
        if (min[i] < 0)
        {
          min[i] = 0;
        }
        if (max[i] >= this->SampleDimensions[i])
        {
          max[i] = this->SampleDimensions[i] - 1;
        }
      }

      // Parallel splat the point
      splat.SetBounds(min, max);
      svtkSMPTools::For(min[2], max[2] + 1, splat);

    } // for all input points
  }   // for all datasets

  // If capping is turned on, set the distances of the outside of the volume
  // to the CapValue.
  //
  if (this->Capping)
  {
    this->Cap(newScalars);
  }

  svtkDebugMacro(<< "Splatted " << totalNumPts << " points");

  // Update self and release memory
  //
  delete[] this->Visited;

  return 1;
}

//----------------------------------------------------------------------------
void svtkGaussianSplatter::ComputeModelBounds(
  svtkCompositeDataSet* input, svtkImageData* output, svtkInformation* outInfo)
{
  double *bounds, maxDist;
  double tempBounds[6] = { 1, -1, 1, -1, 1, -1 };
  int i, adjustBounds = 0;

  // compute model bounds if not set previously
  if (this->ModelBounds[0] >= this->ModelBounds[1] ||
    this->ModelBounds[2] >= this->ModelBounds[3] || this->ModelBounds[4] >= this->ModelBounds[5])
  {
    adjustBounds = 1;
    svtkSmartPointer<svtkCompositeDataIterator> itr =
      svtkSmartPointer<svtkCompositeDataIterator>::Take(input->NewIterator());
    for (itr->InitTraversal(); !itr->IsDoneWithTraversal(); itr->GoToNextItem())
    {
      svtkDataSet* ds = svtkDataSet::SafeDownCast(itr->GetCurrentDataObject());
      if (ds)
      {
        if (tempBounds[0] > tempBounds[1])
        {
          ds->GetBounds(tempBounds);
        }
        else
        {
          const double* dsBounds = ds->GetBounds();
          for (int j = 0; j < 3; ++j)
          {
            tempBounds[2 * j] = std::min(tempBounds[2 * j], dsBounds[2 * j]);
            tempBounds[2 * j + 1] = std::max(tempBounds[2 * j + 1], dsBounds[2 * j + 1]);
          }
        }
      }
    }
    bounds = tempBounds;
  }
  else
  {
    bounds = this->ModelBounds;
  }

  for (maxDist = 0.0, i = 0; i < 3; i++)
  {
    if ((bounds[2 * i + 1] - bounds[2 * i]) > maxDist)
    {
      maxDist = bounds[2 * i + 1] - bounds[2 * i];
    }
  }
  maxDist *= this->Radius;
  this->Radius2 = maxDist * maxDist;

  // adjust bounds so model fits strictly inside (only if not set previously)
  if (adjustBounds)
  {
    for (i = 0; i < 3; i++)
    {
      this->ModelBounds[2 * i] = bounds[2 * i] - maxDist;
      this->ModelBounds[2 * i + 1] = bounds[2 * i + 1] + maxDist;
    }
  }

  // Set volume origin and data spacing
  outInfo->Set(
    svtkDataObject::ORIGIN(), this->ModelBounds[0], this->ModelBounds[2], this->ModelBounds[4]);
  memcpy(this->Origin, outInfo->Get(svtkDataObject::ORIGIN()), sizeof(double) * 3);
  output->SetOrigin(this->Origin);

  for (i = 0; i < 3; i++)
  {
    this->Spacing[i] =
      (this->ModelBounds[2 * i + 1] - this->ModelBounds[2 * i]) / (this->SampleDimensions[i] - 1);
    if (this->Spacing[i] <= 0.0)
    {
      this->Spacing[i] = 1.0;
    }
  }
  outInfo->Set(svtkDataObject::SPACING(), this->Spacing, 3);
  output->SetSpacing(this->Spacing);

  // Determine the splat propagation distance...used later
  for (i = 0; i < 3; i++)
  {
    this->SplatDistance[i] = maxDist / this->Spacing[i];
  }
}

//----------------------------------------------------------------------------
// Compute the size of the sample bounding box automatically from the
// input data.
void svtkGaussianSplatter::ComputeModelBounds(
  svtkDataSet* input, svtkImageData* output, svtkInformation* outInfo)
{
  const double* bounds;
  double maxDist;
  int i, adjustBounds = 0;

  // compute model bounds if not set previously
  if (this->ModelBounds[0] >= this->ModelBounds[1] ||
    this->ModelBounds[2] >= this->ModelBounds[3] || this->ModelBounds[4] >= this->ModelBounds[5])
  {
    adjustBounds = 1;
    bounds = input->GetBounds();
  }
  else
  {
    bounds = this->ModelBounds;
  }

  for (maxDist = 0.0, i = 0; i < 3; i++)
  {
    if ((bounds[2 * i + 1] - bounds[2 * i]) > maxDist)
    {
      maxDist = bounds[2 * i + 1] - bounds[2 * i];
    }
  }
  maxDist *= this->Radius;
  this->Radius2 = maxDist * maxDist;

  // adjust bounds so model fits strictly inside (only if not set previously)
  if (adjustBounds)
  {
    for (i = 0; i < 3; i++)
    {
      this->ModelBounds[2 * i] = bounds[2 * i] - maxDist;
      this->ModelBounds[2 * i + 1] = bounds[2 * i + 1] + maxDist;
    }
  }

  // Set volume origin and data spacing
  outInfo->Set(
    svtkDataObject::ORIGIN(), this->ModelBounds[0], this->ModelBounds[2], this->ModelBounds[4]);
  memcpy(this->Origin, outInfo->Get(svtkDataObject::ORIGIN()), sizeof(double) * 3);
  output->SetOrigin(this->Origin);

  for (i = 0; i < 3; i++)
  {
    this->Spacing[i] =
      (this->ModelBounds[2 * i + 1] - this->ModelBounds[2 * i]) / (this->SampleDimensions[i] - 1);
    if (this->Spacing[i] <= 0.0)
    {
      this->Spacing[i] = 1.0;
    }
  }
  outInfo->Set(svtkDataObject::SPACING(), this->Spacing, 3);
  output->SetSpacing(this->Spacing);

  // Determine the splat propagation distance...used later
  for (i = 0; i < 3; i++)
  {
    this->SplatDistance[i] = maxDist / this->Spacing[i];
  }
}

// Set the dimensions of the sampling structured point set.
void svtkGaussianSplatter::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

//----------------------------------------------------------------------------
void svtkGaussianSplatter::SetSampleDimensions(int dim[3])
{
  int dataDim, i;

  svtkDebugMacro(<< " setting SampleDimensions to (" << dim[0] << "," << dim[1] << "," << dim[2]
                << ")");

  if (dim[0] != this->SampleDimensions[0] || dim[1] != this->SampleDimensions[1] ||
    dim[2] != this->SampleDimensions[2])
  {
    if (dim[0] < 1 || dim[1] < 1 || dim[2] < 1)
    {
      svtkErrorMacro(<< "Bad Sample Dimensions, retaining previous values");
      return;
    }

    for (dataDim = 0, i = 0; i < 3; i++)
    {
      if (dim[i] > 1)
      {
        dataDim++;
      }
    }

    if (dataDim < 3)
    {
      svtkErrorMacro(<< "Sample dimensions must define a volume!");
      return;
    }

    for (i = 0; i < 3; i++)
    {
      this->SampleDimensions[i] = dim[i];
    }

    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkGaussianSplatter::Cap(svtkDoubleArray* s)
{
  int i, j, k;
  svtkIdType idx;
  int d01 = this->SampleDimensions[0] * this->SampleDimensions[1];

  // i-j planes
  // k = 0;
  for (j = 0; j < this->SampleDimensions[1]; j++)
  {
    for (i = 0; i < this->SampleDimensions[0]; i++)
    {
      s->SetTuple(i + j * this->SampleDimensions[0], &this->CapValue);
    }
  }
  k = this->SampleDimensions[2] - 1;
  idx = k * d01;
  for (j = 0; j < this->SampleDimensions[1]; j++)
  {
    for (i = 0; i < this->SampleDimensions[0]; i++)
    {
      s->SetTuple(idx + i + j * this->SampleDimensions[0], &this->CapValue);
    }
  }
  // j-k planes
  // i = 0;
  for (k = 0; k < this->SampleDimensions[2]; k++)
  {
    for (j = 0; j < this->SampleDimensions[1]; j++)
    {
      s->SetTuple(j * this->SampleDimensions[0] + k * d01, &this->CapValue);
    }
  }
  i = this->SampleDimensions[0] - 1;
  for (k = 0; k < this->SampleDimensions[2]; k++)
  {
    for (j = 0; j < this->SampleDimensions[1]; j++)
    {
      s->SetTuple(i + j * this->SampleDimensions[0] + k * d01, &this->CapValue);
    }
  }
  // i-k planes
  // j = 0;
  for (k = 0; k < this->SampleDimensions[2]; k++)
  {
    for (i = 0; i < this->SampleDimensions[0]; i++)
    {
      s->SetTuple(i + k * d01, &this->CapValue);
    }
  }
  j = this->SampleDimensions[1] - 1;
  idx = j * this->SampleDimensions[0];
  for (k = 0; k < this->SampleDimensions[2]; k++)
  {
    for (i = 0; i < this->SampleDimensions[0]; i++)
    {
      s->SetTuple(idx + i + k * d01, &this->CapValue);
    }
  }
}

//----------------------------------------------------------------------------
//
//  Gaussian sampling
//
double svtkGaussianSplatter::Gaussian(double cx[3])
{
  return ((cx[0] - P[0]) * (cx[0] - P[0]) + (cx[1] - P[1]) * (cx[1] - P[1]) +
    (cx[2] - P[2]) * (cx[2] - P[2]));
}

//----------------------------------------------------------------------------
//
//  Ellipsoidal Gaussian sampling
//
double svtkGaussianSplatter::EccentricGaussian(double cx[3])
{
  double v[3], r2, z2, rxy2, mag;

  v[0] = cx[0] - this->P[0];
  v[1] = cx[1] - this->P[1];
  v[2] = cx[2] - this->P[2];

  r2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

  if ((mag = this->N[0] * this->N[0] + this->N[1] * this->N[1] + this->N[2] * this->N[2]) != 1.0)
  {
    if (mag == 0.0)
    {
      mag = 1.0;
    }
    else
    {
      mag = sqrt(mag);
    }
  }

  z2 = (v[0] * this->N[0] + v[1] * this->N[1] + v[2] * this->N[2]) / mag;
  z2 = z2 * z2;

  rxy2 = r2 - z2;

  return (rxy2 / this->Eccentricity2 + z2);
}

//----------------------------------------------------------------------------
const char* svtkGaussianSplatter::GetAccumulationModeAsString()
{
  if (this->AccumulationMode == SVTK_ACCUMULATION_MODE_MIN)
  {
    return "Minimum";
  }
  else if (this->AccumulationMode == SVTK_ACCUMULATION_MODE_MAX)
  {
    return "Maximum";
  }
  else // if ( this->AccumulationMode == SVTK_ACCUMULATION_MODE_SUM )
  {
    return "Sum";
  }
}

//----------------------------------------------------------------------------
void svtkGaussianSplatter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
     << this->SampleDimensions[1] << ", " << this->SampleDimensions[2] << ")\n";

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Exponent Factor: " << this->ExponentFactor << "\n";

  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "Normal Warping: " << (this->NormalWarping ? "On\n" : "Off\n");
  os << indent << "Eccentricity: " << this->Eccentricity << "\n";

  os << indent << "Scalar Warping: " << (this->ScalarWarping ? "On\n" : "Off\n");
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Cap Value: " << this->CapValue << "\n";

  os << indent << "Accumulation Mode: " << this->GetAccumulationModeAsString() << "\n";

  os << indent << "Null Value: " << this->NullValue << "\n";
}

//----------------------------------------------------------------------------
int svtkGaussianSplatter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkCompositeDataSet");
  return 1;
}
