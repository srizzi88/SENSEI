/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnsignedDistance.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkUnsignedDistance.h"

#include "svtkAbstractPointLocator.h"
#include "svtkArrayListTemplate.h" // For processing attribute data
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStaticPointLocator.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkUnsignedDistance);
svtkCxxSetObjectMacro(svtkUnsignedDistance, Locator, svtkAbstractPointLocator);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

// The threaded core of the algorithm
template <typename TS>
struct UnsignedDistance
{
  svtkIdType Dims[3];
  double Origin[3];
  double Spacing[3];
  double Radius;
  svtkAbstractPointLocator* Locator;
  TS* Scalars;

  UnsignedDistance(int dims[3], double origin[3], double spacing[3], double radius,
    svtkAbstractPointLocator* loc, TS* scalars)
    : Radius(radius)
    , Locator(loc)
    , Scalars(scalars)
  {
    for (int i = 0; i < 3; ++i)
    {
      this->Dims[i] = static_cast<svtkIdType>(dims[i]);
      this->Origin[i] = origin[i];
      this->Spacing[i] = spacing[i];
    }
  }

  // Threaded interpolation method
  void operator()(svtkIdType slice, svtkIdType sliceEnd)
  {
    double x[3], dist2;
    const double radius = this->Radius;
    double* origin = this->Origin;
    double* spacing = this->Spacing;
    svtkIdType* dims = this->Dims;
    svtkIdType ptId, closest, jOffset, kOffset, sliceSize = dims[0] * dims[1];

    for (; slice < sliceEnd; ++slice)
    {
      x[2] = origin[2] + slice * spacing[2];
      kOffset = slice * sliceSize;

      for (svtkIdType j = 0; j < dims[1]; ++j)
      {
        x[1] = origin[1] + j * spacing[1];
        jOffset = j * dims[0];

        for (svtkIdType i = 0; i < dims[0]; ++i)
        {
          x[0] = origin[0] + i * spacing[0];
          ptId = i + jOffset + kOffset;

          // Compute signed distance from surrounding points
          closest = this->Locator->FindClosestPointWithinRadius(radius, x, dist2);
          if (closest >= 0)
          {
            this->Scalars[ptId] = sqrt(dist2);
          } // if nearby points
        }   // over i
      }     // over j
    }       // over slices
  }

  static void Execute(
    svtkUnsignedDistance* self, int dims[3], double origin[3], double spacing[3], TS* scalars)
  {
    UnsignedDistance dist(dims, origin, spacing, self->GetRadius(), self->GetLocator(), scalars);
    svtkSMPTools::For(0, dims[2], dist);
  }

}; // UnsignedDistance

// Compute ModelBounds from input geometry. Return if the model bounds is
// already set.
void ComputeModelBounds(svtkPolyData* input, int dims[3], int adjustBounds, double adjustDistance,
  double modelBounds[6], double origin[3], double spacing[3])
{
  int i;

  // compute model bounds if not set previously
  if (input == nullptr ||
    (modelBounds[0] < modelBounds[1] && modelBounds[2] < modelBounds[3] &&
      modelBounds[4] < modelBounds[5]))
  {
    ; // do nothing
  }

  else // automatically adjust the bounds
  {
    double bounds[6];
    input->GetBounds(bounds);
    double maxDist = 0.0;
    for (i = 0; i < 3; i++)
    {
      if ((bounds[2 * i + 1] - bounds[2 * i]) > maxDist)
      {
        maxDist = bounds[2 * i + 1] - bounds[2 * i];
      }
    }

    // adjust bounds so model fits strictly inside (only if not set previously)
    maxDist = (adjustBounds ? adjustDistance * maxDist : 0.0);
    for (i = 0; i < 3; i++)
    {
      modelBounds[2 * i] = bounds[2 * i] - maxDist;
      modelBounds[2 * i + 1] = bounds[2 * i + 1] + maxDist;
    }
  }

  // Compute the final pieces of information
  for (i = 0; i < 3; ++i)
  {
    origin[i] = modelBounds[2 * i];
    spacing[i] = (modelBounds[2 * i + 1] - modelBounds[2 * i]) / static_cast<double>(dims[i] - 1);
  }
}

// If requested, cap the outer values of the volume
template <typename T>
void Cap(int dims[3], T* s, double capValue)
{
  int i, j, k;
  int idx;
  int d01 = dims[0] * dims[1];

  // i-j planes
  for (j = 0; j < dims[1]; j++)
  {
    for (i = 0; i < dims[0]; i++)
    {
      s[i + j * dims[0]] = capValue;
    }
  }
  k = dims[2] - 1;
  idx = k * d01;
  for (j = 0; j < dims[1]; j++)
  {
    for (i = 0; i < dims[0]; i++)
    {
      s[idx + i + j * dims[0]] = capValue;
    }
  }
  // j-k planes
  for (k = 0; k < dims[2]; k++)
  {
    for (j = 0; j < dims[1]; j++)
    {
      s[j * dims[0] + k * d01] = capValue;
    }
  }
  i = dims[0] - 1;
  for (k = 0; k < dims[2]; k++)
  {
    for (j = 0; j < dims[1]; j++)
    {
      s[i + j * dims[0] + k * d01] = capValue;
    }
  }
  // i-k planes
  for (k = 0; k < dims[2]; k++)
  {
    for (i = 0; i < dims[0]; i++)
    {
      s[i + k * d01] = capValue;
    }
  }
  j = dims[1] - 1;
  idx = j * dims[0];
  for (k = 0; k < dims[2]; k++)
  {
    for (i = 0; i < dims[0]; i++)
    {
      s[idx + i + k * d01] = capValue;
    }
  }
}

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
// Construct with sample dimensions=(256,256,256), and so that model bounds are
// automatically computed from the input.
svtkUnsignedDistance::svtkUnsignedDistance()
{
  this->Dimensions[0] = 256;
  this->Dimensions[1] = 256;
  this->Dimensions[2] = 256;

  this->Bounds[0] = 0.0;
  this->Bounds[1] = 0.0;
  this->Bounds[2] = 0.0;
  this->Bounds[3] = 0.0;
  this->Bounds[4] = 0.0;
  this->Bounds[5] = 0.0;
  this->AdjustBounds = 1;
  this->AdjustDistance = 0.0125;

  this->Radius = 0.1;

  this->Capping = 1;
  this->OutputScalarType = SVTK_FLOAT;
  this->CapValue = SVTK_FLOAT_MAX;

  this->Locator = svtkStaticPointLocator::New();

  this->Initialized = 0;
}

//----------------------------------------------------------------------------
svtkUnsignedDistance::~svtkUnsignedDistance()
{
  this->SetLocator(nullptr);
}

//----------------------------------------------------------------------------
// Initialize the filter for appending data. You must invoke the
// StartAppend() method before doing successive Appends(). It's also a
// good idea to manually specify the model bounds; otherwise the input
// bounds for the data will be used.
void svtkUnsignedDistance::StartAppend()
{
  svtkInformation* outInfo = this->GetOutputInformation(0);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    svtkStreamingDemandDrivenPipeline::GetWholeExtent(outInfo), 6);

  svtkDebugMacro(<< "Initializing data");
  this->AllocateOutputData(this->GetOutput(), this->GetOutputInformation(0));
  svtkIdType numPts = static_cast<svtkIdType>(this->Dimensions[0]) *
    static_cast<svtkIdType>(this->Dimensions[1]) * static_cast<svtkIdType>(this->Dimensions[2]);

  // initialize output to initial unseen value at each location
  if (this->OutputScalarType == SVTK_DOUBLE)
  {
    double* newScalars =
      static_cast<double*>(this->GetOutput()->GetPointData()->GetScalars()->GetVoidPointer(0));
    std::fill_n(newScalars, numPts, static_cast<double>(this->CapValue));
  }
  else
  {
    float* newScalars =
      static_cast<float*>(this->GetOutput()->GetPointData()->GetScalars()->GetVoidPointer(0));
    std::fill_n(newScalars, numPts, static_cast<float>(this->CapValue));
  }

  // Compute the initial bounds if required
  double origin[3], spacing[3];
  svtkImageData* output = this->GetOutput();

  // compute model bounds if not set previously
  svtkPolyData* input = svtkPolyData::SafeDownCast(this->GetInput());
  ComputeModelBounds(input, this->Dimensions, this->AdjustBounds, this->AdjustDistance,
    this->Bounds, origin, spacing);

  // Set volume origin and data spacing
  output->SetOrigin(origin);
  output->SetSpacing(spacing);

  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);

  this->Initialized = 1;
}

//----------------------------------------------------------------------------
// Append a data set to the existing output. To use this function,
// you'll have to invoke the StartAppend() method before doing
// successive appends. It's also a good idea to specify the model
// bounds; otherwise the input model bounds is used. When you've
// finished appending, use the EndAppend() method.
void svtkUnsignedDistance::Append(svtkPolyData* input)
{
  svtkDebugMacro(<< "Appending data");

  // There better be data
  if (!input || input->GetNumberOfPoints() < 1)
  {
    return;
  }

  if (!this->Initialized)
  {
    this->StartAppend();
  }

  // Set up for processing
  svtkDataArray* image = this->GetOutput()->GetPointData()->GetScalars();
  void* scalars = image->GetVoidPointer(0);

  // Build the locator
  if (!this->Locator)
  {
    svtkErrorMacro(<< "Point locator required\n");
    return;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Finally: compute the signed distance function
  svtkImageData* output = this->GetOutput();
  switch (image->GetDataType())
  {
    svtkTemplateMacro(UnsignedDistance<SVTK_TT>::Execute(
      this, this->Dimensions, output->GetOrigin(), output->GetSpacing(), (SVTK_TT*)scalars));
  }
}

//----------------------------------------------------------------------------
// Method completes the append process (does the capping if requested).
void svtkUnsignedDistance::EndAppend()
{
  svtkDataArray* image;
  svtkDebugMacro(<< "End append");

  if (!(image = this->GetOutput()->GetPointData()->GetScalars()))
  {
    svtkErrorMacro("No output produced.");
    return;
  }

  // Cap volume if requested
  if (this->Capping)
  {
    void* scalars = image->GetVoidPointer(0);

    // Finally: compute the signed distance function
    switch (image->GetDataType())
    {
      svtkTemplateMacro(Cap<SVTK_TT>(this->Dimensions, (SVTK_TT*)scalars, this->CapValue));
    }
  }
}

//----------------------------------------------------------------------------
int svtkUnsignedDistance::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  double spacing[3], origin[3];

  if (this->OutputScalarType == SVTK_DOUBLE)
  {
    svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_DOUBLE, 1);
  }
  else
  {
    svtkDataObject::SetPointDataActiveScalarInfo(outInfo, SVTK_FLOAT, 1);
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 0, this->Dimensions[0] - 1, 0,
    this->Dimensions[1] - 1, 0, this->Dimensions[2] - 1);

  ComputeModelBounds(nullptr, this->Dimensions, this->AdjustBounds, this->AdjustDistance,
    this->Bounds, origin, spacing);

  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);

  return 1;
}

//----------------------------------------------------------------------------
int svtkUnsignedDistance::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // get the input
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Executing space carver");

  if (input == nullptr)
  {
    // we do not want to release the data because user might
    // have called Append ...
    return 0;
  }

  this->StartAppend();
  this->Append(input);
  this->EndAppend();

  return 1;
}

//----------------------------------------------------------------------------
// Set the i-j-k dimensions on which to sample the distance function.
void svtkUnsignedDistance::SetDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetDimensions(dim);
}

//----------------------------------------------------------------------------
void svtkUnsignedDistance::SetDimensions(const int dim[3])
{
  int dataDim, i;

  svtkDebugMacro(<< " setting Dimensions to (" << dim[0] << "," << dim[1] << "," << dim[2] << ")");

  if (dim[0] != this->Dimensions[0] || dim[1] != this->Dimensions[1] ||
    dim[2] != this->Dimensions[2])
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
      this->Dimensions[i] = dim[i];
    }

    this->Modified();
  }
}

//----------------------------------------------------------------------------
int svtkUnsignedDistance::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkUnsignedDistance::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // If we have no input then we will not generate the output because
  // the user already called StartAppend/Append/EndAppend.
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_NOT_GENERATED()))
  {
    if (inputVector[0]->GetNumberOfInformationObjects() == 0)
    {
      svtkInformation* outInfo = outputVector->GetInformationObject(0);
      outInfo->Set(svtkDemandDrivenPipeline::DATA_NOT_GENERATED(), 1);
    }
    return 1;
  }
  else if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    if (inputVector[0]->GetNumberOfInformationObjects() == 0)
    {
      return 1;
    }
  }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void svtkUnsignedDistance::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Dimensions: (" << this->Dimensions[0] << ", " << this->Dimensions[1] << ", "
     << this->Dimensions[2] << ")\n";

  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->Bounds[0] << ", " << this->Bounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->Bounds[2] << ", " << this->Bounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->Bounds[4] << ", " << this->Bounds[5] << ")\n";

  os << indent << "Adjust Bounds: " << (this->AdjustBounds ? "On\n" : "Off\n");
  os << indent << "Adjust Distance: " << this->AdjustDistance << "\n";

  os << indent << "Radius: " << this->Radius << "\n";

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");
  os << indent << "Cap Value: " << this->CapValue << "\n";

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";

  os << indent << "Locator: " << this->Locator << "\n";
}
