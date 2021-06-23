/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShepardMethod.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkShepardMethod.h"

#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSMPTools.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkShepardMethod);

//-----------------------------------------------------------------------------
// Thread the algorithm by processing each z-slice independently as each
// point is procssed. (As input points are processed, their influence is felt
// across a cuboid domain - a splat footprint. The slices that make up the
// cuboid splat are processed in parallel.) Note also that the scalar data is
// processed via templating.
class svtkShepardAlgorithm
{
public:
  int* Dims;
  svtkIdType SliceSize;
  double *Origin, *Spacing;
  float* OutScalars;
  double* Sum;

  svtkShepardAlgorithm(double* origin, double* spacing, int* dims, float* outS, double* sum)
    : Dims(dims)
    , Origin(origin)
    , Spacing(spacing)
    , OutScalars(outS)
    , Sum(sum)
  {
    this->SliceSize = this->Dims[0] * this->Dims[1];
  }

  class SplatP2
  {
  public:
    svtkShepardAlgorithm* Algo;
    svtkIdType XMin, XMax, YMin, YMax, ZMin, ZMax;
    double S, X[3];
    SplatP2(svtkShepardAlgorithm* algo)
      : Algo(algo)
    {
    }
    void SetBounds(svtkIdType min[3], svtkIdType max[3])
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
      double cx[3], distance2, *sum = this->Algo->Sum;
      float* outS = this->Algo->OutScalars;
      const double* origin = this->Algo->Origin;
      const double* spacing = this->Algo->Spacing;
      for (; slice < end; ++slice)
      {
        // Loop over all sample points in volume within footprint and
        // evaluate the splat
        cx[2] = origin[2] + spacing[2] * slice;
        kOffset = slice * this->Algo->SliceSize;
        for (j = this->YMin; j <= this->YMax; j++)
        {
          cx[1] = origin[1] + spacing[1] * j;
          jOffset = j * this->Algo->Dims[0];
          for (i = this->XMin; i <= this->XMax; i++)
          {
            idx = kOffset + jOffset + i;
            cx[0] = origin[0] + spacing[0] * i;

            distance2 = svtkMath::Distance2BetweenPoints(this->X, cx);

            // When the sample point and interpolated point are coincident,
            // then the interpolated point takes on the value of the sample
            // point.
            if (distance2 == 0.0)
            {
              sum[idx] = SVTK_DOUBLE_MAX; // mark the point as hit
              outS[idx] = this->S;
            }
            else if (sum[idx] < SVTK_DOUBLE_MAX)
            {
              sum[idx] += 1.0 / distance2;
              outS[idx] += this->S / distance2;
            }

          } // i
        }   // j
      }     // k within splat footprint
    }
  };

  class SplatPN
  {
  public:
    svtkShepardAlgorithm* Algo;
    svtkIdType XMin, XMax, YMin, YMax, ZMin, ZMax;
    double P, S, X[3];
    SplatPN(svtkShepardAlgorithm* algo, double p)
      : Algo(algo)
      , P(p)
    {
    }
    void SetBounds(svtkIdType min[3], svtkIdType max[3])
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
      double cx[3], distance, dp, *sum = this->Algo->Sum;
      float* outS = this->Algo->OutScalars;
      const double* origin = this->Algo->Origin;
      const double* spacing = this->Algo->Spacing;
      for (; slice < end; ++slice)
      {
        // Loop over all sample points in volume within footprint and
        // evaluate the splat
        cx[2] = origin[2] + spacing[2] * slice;
        kOffset = slice * this->Algo->SliceSize;
        for (j = this->YMin; j <= this->YMax; j++)
        {
          cx[1] = origin[1] + spacing[1] * j;
          jOffset = j * this->Algo->Dims[0];
          for (i = this->XMin; i <= this->XMax; i++)
          {
            idx = kOffset + jOffset + i;
            cx[0] = origin[0] + spacing[0] * i;

            distance = sqrt(svtkMath::Distance2BetweenPoints(this->X, cx));

            // When the sample point and interpolated point are coincident,
            // then the interpolated point takes on the value of the sample
            // point.
            if (distance == 0.0)
            {
              sum[idx] = SVTK_DOUBLE_MAX; // mark the point as hit
              outS[idx] = this->S;
            }
            else if (sum[idx] < SVTK_DOUBLE_MAX)
            {
              dp = pow(distance, this->P);
              sum[idx] += 1.0 / dp;
              outS[idx] += this->S / dp;
            }

          } // i
        }   // j
      }     // k within splat footprint
    }
  };

  class Interpolate
  {
  public:
    svtkShepardAlgorithm* Algo;
    double NullValue;
    Interpolate(svtkShepardAlgorithm* algo, double nullV)
      : Algo(algo)
      , NullValue(nullV)
    {
    }
    void operator()(svtkIdType ptId, svtkIdType endPtId)
    {
      float* outS = this->Algo->OutScalars;
      const double* sum = this->Algo->Sum;
      for (; ptId < endPtId; ++ptId)
      {
        if (sum[ptId] >= SVTK_DOUBLE_MAX)
        {
          ; // previously set, precise hit
        }
        else if (sum[ptId] != 0.0)
        {
          outS[ptId] /= sum[ptId];
        }
        else
        {
          outS[ptId] = this->NullValue;
        }
      }
    }
  };
}; // Shepard algorithm

//-----------------------------------------------------------------------------
// Construct with sample dimensions=(50,50,50) and so that model bounds are
// automatically computed from input. Null value for each unvisited output
// point is 0.0. Maximum distance is 0.25.
svtkShepardMethod::svtkShepardMethod()
{
  this->MaximumDistance = 0.25;

  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;

  this->SampleDimensions[0] = 50;
  this->SampleDimensions[1] = 50;
  this->SampleDimensions[2] = 50;

  this->NullValue = 0.0;

  this->PowerParameter = 2.0;
}

//-----------------------------------------------------------------------------
// Compute ModelBounds from input geometry.
double svtkShepardMethod::ComputeModelBounds(double origin[3], double spacing[3])
{
  const double* bounds;
  double maxDist;
  int i, adjustBounds = 0;

  // compute model bounds if not set previously
  if (this->ModelBounds[0] >= this->ModelBounds[1] ||
    this->ModelBounds[2] >= this->ModelBounds[3] || this->ModelBounds[4] >= this->ModelBounds[5])
  {
    adjustBounds = 1;
    svtkDataSet* ds = svtkDataSet::SafeDownCast(this->GetInput());
    // ds better be non null otherwise something is very wrong here
    bounds = ds->GetBounds();
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
  maxDist *= this->MaximumDistance;

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
  for (i = 0; i < 3; i++)
  {
    origin[i] = this->ModelBounds[2 * i];
    spacing[i] =
      (this->ModelBounds[2 * i + 1] - this->ModelBounds[2 * i]) / (this->SampleDimensions[i] - 1);
  }

  return maxDist;
}

//-----------------------------------------------------------------------------
int svtkShepardMethod::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int i;
  double ar[3], origin[3];

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, this->SampleDimensions[0] - 1,
    0, this->SampleDimensions[1] - 1, 0, this->SampleDimensions[2] - 1);

  for (i = 0; i < 3; i++)
  {
    origin[i] = this->ModelBounds[2 * i];
    if (this->SampleDimensions[i] <= 1)
    {
      ar[i] = 1;
    }
    else
    {
      ar[i] =
        (this->ModelBounds[2 * i + 1] - this->ModelBounds[2 * i]) / (this->SampleDimensions[i] - 1);
    }
  }
  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
  outInfo->Set(svtkDataObject::SPACING(), ar, 3);

  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_FLOAT, 1);
  return 1;
}

//-----------------------------------------------------------------------------
int svtkShepardMethod::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the input
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  // get the output
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* output = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  // We need to allocate our own scalars since we are overriding
  // the superclasses "Execute()" method.
  output->SetExtent(outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  output->AllocateScalars(outInfo);

  svtkIdType ptId, i;
  double *sum, spacing[3], origin[3];
  double maxDistance;
  svtkDataArray* inScalars;
  svtkIdType numPts, numNewPts;
  svtkIdType min[3], max[3];
  svtkFloatArray* newScalars = svtkArrayDownCast<svtkFloatArray>(output->GetPointData()->GetScalars());

  svtkDebugMacro(<< "Executing Shepard method");

  // Check input
  //
  if ((numPts = input->GetNumberOfPoints()) < 1)
  {
    svtkErrorMacro(<< "Points must be defined!");
    return 1;
  }

  if ((inScalars = input->GetPointData()->GetScalars()) == nullptr)
  {
    svtkErrorMacro(<< "Scalars must be defined!");
    return 1;
  }
  float* newS = static_cast<float*>(newScalars->GetVoidPointer(0));

  newScalars->SetName(inScalars->GetName());

  // Allocate and set up output
  //
  numNewPts = this->SampleDimensions[0] * this->SampleDimensions[1] * this->SampleDimensions[2];

  sum = new double[numNewPts];
  std::fill_n(sum, numNewPts, 0.0);
  std::fill_n(newS, numNewPts, 0.0);

  maxDistance = this->ComputeModelBounds(origin, spacing);
  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);

  // Could easily be templated for output scalar type
  svtkShepardAlgorithm algo(origin, spacing, this->SampleDimensions, newS, sum);

  // Traverse all input points. Depending on power parameter
  // different paths are taken.
  //
  if (this->PowerParameter == 2.0) // distance2
  {
    svtkShepardAlgorithm::SplatP2 splatF(&algo);
    for (ptId = 0; ptId < numPts; ptId++)
    {
      if (!(ptId % 1000))
      {
        svtkDebugMacro(<< "Inserting point #" << ptId);
        this->UpdateProgress(ptId / numPts);
        if (this->GetAbortExecute())
        {
          break;
        }
      }

      input->GetPoint(ptId, splatF.X);
      splatF.S = inScalars->GetComponent(ptId, 0);

      for (i = 0; i < 3; i++) // compute dimensional bounds in data set
      {
        min[i] = static_cast<int>(
          static_cast<double>((splatF.X[i] - maxDistance) - origin[i]) / spacing[i]);
        max[i] = static_cast<int>(
          static_cast<double>((splatF.X[i] + maxDistance) - origin[i]) / spacing[i]);
        min[i] = (min[i] < 0 ? 0 : min[i]);
        max[i] = (max[i] >= this->SampleDimensions[i] ? this->SampleDimensions[i] - 1 : max[i]);
      }

      splatF.SetBounds(min, max);
      svtkSMPTools::For(min[2], max[2] + 1, splatF);
    }
  } // power parameter p=2

  else // have to take roots etc so it runs slower
  {
    svtkShepardAlgorithm::SplatPN splatF(&algo, this->PowerParameter);
    for (ptId = 0; ptId < numPts; ptId++)
    {
      if (!(ptId % 1000))
      {
        svtkDebugMacro(<< "Inserting point #" << ptId);
        this->UpdateProgress(ptId / numPts);
        if (this->GetAbortExecute())
        {
          break;
        }
      }

      input->GetPoint(ptId, splatF.X);
      splatF.S = inScalars->GetComponent(ptId, 0);

      for (i = 0; i < 3; i++) // compute dimensional bounds in data set
      {
        min[i] = static_cast<int>(
          static_cast<double>((splatF.X[i] - maxDistance) - origin[i]) / spacing[i]);
        max[i] = static_cast<int>(
          static_cast<double>((splatF.X[i] + maxDistance) - origin[i]) / spacing[i]);
        min[i] = (min[i] < 0 ? 0 : min[i]);
        max[i] = (max[i] >= this->SampleDimensions[i] ? this->SampleDimensions[i] - 1 : max[i]);
      }

      splatF.SetBounds(min, max);
      svtkSMPTools::For(min[2], max[2] + 1, splatF);
    }
  } // p != 2

  // Run through scalars and compute final values
  //
  svtkShepardAlgorithm::Interpolate interpolate(&algo, this->NullValue);
  svtkSMPTools::For(0, numNewPts, interpolate);

  // Clean up
  //
  delete[] sum;

  return 1;
}

//-----------------------------------------------------------------------------
// Set the i-j-k dimensions on which to sample the distance function.
void svtkShepardMethod::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

//-----------------------------------------------------------------------------
// Set the i-j-k dimensions on which to sample the distance function.
void svtkShepardMethod::SetSampleDimensions(int dim[3])
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
      svtkErrorMacro(<< "Sample dimensions must define a 3D volume!");
      return;
    }

    for (i = 0; i < 3; i++)
    {
      this->SampleDimensions[i] = dim[i];
    }

    this->Modified();
  }
}

//-----------------------------------------------------------------------------
int svtkShepardMethod::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
void svtkShepardMethod::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Maximum Distance: " << this->MaximumDistance << "\n";

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
     << this->SampleDimensions[1] << ", " << this->SampleDimensions[2] << ")\n";

  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "Null Value: " << this->NullValue << "\n";

  os << indent << "Power Parameter: " << this->PowerParameter << "\n";
}
