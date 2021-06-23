/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointDensityFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPointDensityFilter.h"

#include "svtkAbstractPointLocator.h"
#include "svtkArrayListTemplate.h" // For processing attribute data
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
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
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"

svtkStandardNewMacro(svtkPointDensityFilter);
svtkCxxSetObjectMacro(svtkPointDensityFilter, Locator, svtkAbstractPointLocator);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm. Operator() processes slices.
struct ComputePointDensity
{
  int Dims[3];
  double Origin[3];
  double Spacing[3];
  float* Density;
  svtkAbstractPointLocator* Locator;
  double Radius, Volume;
  int Form;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage lots of new/delete.
  svtkSMPThreadLocalObject<svtkIdList> PIds;

  ComputePointDensity(int dims[3], double origin[3], double spacing[3], float* dens,
    svtkAbstractPointLocator* loc, double radius, int form)
    : Density(dens)
    , Locator(loc)
    , Radius(radius)
    , Form(form)
  {
    for (int i = 0; i < 3; ++i)
    {
      this->Dims[i] = dims[i];
      this->Origin[i] = origin[i];
      this->Spacing[i] = spacing[i];
    }
    this->Volume = (4.0 / 3.0) * svtkMath::Pi() * radius * radius * radius;
  }

  // Just allocate a little bit of memory to get started.
  void Initialize()
  {
    svtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); // allocate some memory
  }

  void operator()(svtkIdType slice, svtkIdType sliceEnd)
  {
    svtkIdList*& pIds = this->PIds.Local();
    double* origin = this->Origin;
    double* spacing = this->Spacing;
    int* dims = this->Dims;
    double x[3];
    svtkIdType numPts, sliceSize = dims[0] * dims[1];
    float* dens = this->Density + slice * sliceSize;
    double radius = this->Radius;
    double volume = this->Volume;
    svtkAbstractPointLocator* locator = this->Locator;
    int form = this->Form;

    for (; slice < sliceEnd; ++slice)
    {
      x[2] = origin[2] + slice * spacing[2];
      for (int j = 0; j < dims[1]; ++j)
      {
        x[1] = origin[1] + j * spacing[1];
        for (int i = 0; i < dims[0]; ++i)
        {
          x[0] = origin[0] + i * spacing[0];
          // Retrieve the local neighborhood
          locator->FindPointsWithinRadius(radius, x, pIds);
          numPts = pIds->GetNumberOfIds();

          if (form == SVTK_DENSITY_FORM_NPTS)
          {
            *dens++ = static_cast<float>(numPts);
          }
          else // SVTK_DENSITY_FORM_VOLUME_NORM
          {
            *dens++ = static_cast<float>(numPts) / volume;
          }
        } // over i
      }   // over j
    }     // over slices
  }

  void Reduce() {}

  static void Execute(svtkPointDensityFilter* self, int dims[3], double origin[3], double spacing[3],
    float* density, double radius, int form)
  {
    ComputePointDensity compDens(dims, origin, spacing, density, self->GetLocator(), radius, form);
    svtkSMPTools::For(0, dims[2], compDens);
  }
}; // ComputePointDensity

//----------------------------------------------------------------------------
// The threaded core of the algorithm; processes weighted points.
template <typename T>
struct ComputeWeightedDensity : public ComputePointDensity
{
  const T* Weights;

  ComputeWeightedDensity(T* weights, int dims[3], double origin[3], double spacing[3], float* dens,
    svtkAbstractPointLocator* loc, double radius, int form)
    : ComputePointDensity(dims, origin, spacing, dens, loc, radius, form)
    , Weights(weights)
  {
  }

  void operator()(svtkIdType slice, svtkIdType sliceEnd)
  {
    svtkIdList*& pIds = this->PIds.Local();
    double* origin = this->Origin;
    double* spacing = this->Spacing;
    int* dims = this->Dims;
    double x[3];
    svtkIdType sample, numPts, sliceSize = dims[0] * dims[1];
    float* dens = this->Density + slice * sliceSize;
    double radius = this->Radius;
    double volume = this->Volume;
    svtkAbstractPointLocator* locator = this->Locator;
    int form = this->Form;
    double d;
    const T* weights = this->Weights;

    for (; slice < sliceEnd; ++slice)
    {
      x[2] = origin[2] + slice * spacing[2];
      for (int j = 0; j < dims[1]; ++j)
      {
        x[1] = origin[1] + j * spacing[1];
        for (int i = 0; i < dims[0]; ++i)
        {
          x[0] = origin[0] + i * spacing[0];
          // Retrieve the local neighborhood
          locator->FindPointsWithinRadius(radius, x, pIds);
          numPts = pIds->GetNumberOfIds();
          for (d = 0.0, sample = 0; sample < numPts; ++sample)
          {
            d += *(weights + pIds->GetId(sample));
          }

          if (form == SVTK_DENSITY_FORM_NPTS)
          {
            *dens++ = static_cast<float>(d);
          }
          else // SVTK_DENSITY_FORM_VOLUME_NORM
          {
            *dens++ = static_cast<float>(d) / volume;
          }
        } // over i
      }   // over j
    }     // over slices
  }

  void Reduce() {}

  static void Execute(svtkPointDensityFilter* self, T* weights, int dims[3], double origin[3],
    double spacing[3], float* density, double radius, int form)
  {
    ComputeWeightedDensity compDens(
      weights, dims, origin, spacing, density, self->GetLocator(), radius, form);
    svtkSMPTools::For(0, dims[2], compDens);
  }
}; // ComputeWeightedDensity

//----------------------------------------------------------------------------
// Optional kernel to compute gradient of density function. Also the gradient
// magnitude and function classification is computed.
struct ComputeGradients
{
  int Dims[3];
  double Origin[3];
  double Spacing[3];
  float* Density;
  float* Gradients;
  float* GradientMag;
  unsigned char* FuncClassification;

  ComputeGradients(int dims[3], double origin[3], double spacing[3], float* dens, float* grad,
    float* mag, unsigned char* fclass)
    : Density(dens)
    , Gradients(grad)
    , GradientMag(mag)
    , FuncClassification(fclass)

  {
    for (int i = 0; i < 3; ++i)
    {
      this->Dims[i] = dims[i];
      this->Origin[i] = origin[i];
      this->Spacing[i] = spacing[i];
    }
  }

  void operator()(svtkIdType slice, svtkIdType sliceEnd)
  {
    double* spacing = this->Spacing;
    int* dims = this->Dims;
    svtkIdType sliceSize = dims[0] * dims[1];
    float* d = this->Density + slice * sliceSize;
    float* grad = this->Gradients + 3 * slice * sliceSize;
    float* mag = this->GradientMag + slice * sliceSize;
    unsigned char* fclass = this->FuncClassification + slice * sliceSize;
    float f, dp, dm;
    bool nonZeroComp;
    int idx[3], incs[3];
    incs[0] = 1;
    incs[1] = dims[0];
    incs[2] = sliceSize;

    for (; slice < sliceEnd; ++slice)
    {
      idx[2] = slice;
      for (int j = 0; j < dims[1]; ++j)
      {
        idx[1] = j;
        for (int i = 0; i < dims[0]; ++i)
        {
          idx[0] = i;
          nonZeroComp = false;
          for (int ii = 0; ii < 3; ++ii)
          {
            if (idx[ii] == 0)
            {
              dm = *d;
              dp = *(d + incs[ii]);
              f = 1.0;
            }
            else if (idx[ii] == dims[ii] - 1)
            {
              dm = *(d - incs[ii]);
              dp = *d;
              f = 1.0;
            }
            else
            {
              dm = *(d - incs[ii]);
              dp = *(d + incs[ii]);
              f = 0.5;
            }
            grad[ii] = f * (dp - dm) / spacing[ii];
            nonZeroComp = ((dp != 0.0 || dm != 0.0) ? true : nonZeroComp);
          }

          // magnitude
          if (nonZeroComp)
          {
            *mag++ = svtkMath::Norm(grad);
            *fclass++ = svtkPointDensityFilter::NON_ZERO;
          }
          else
          {
            *mag++ = 0.0;
            *fclass++ = svtkPointDensityFilter::ZERO;
          }
          d++;
          grad += 3;

        } // over i
      }   // over j
    }     // over slices
  }

  static void Execute(int dims[3], double origin[3], double spacing[3], float* density, float* grad,
    float* mag, unsigned char* fclass)
  {
    ComputeGradients compGrad(dims, origin, spacing, density, grad, mag, fclass);
    svtkSMPTools::For(0, dims[2], compGrad);
  }
}; // ComputeGradients

} // anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
svtkPointDensityFilter::svtkPointDensityFilter()
{
  this->SampleDimensions[0] = 100;
  this->SampleDimensions[1] = 100;
  this->SampleDimensions[2] = 100;

  // All of these zeros mean automatic computation
  this->ModelBounds[0] = 0.0;
  this->ModelBounds[1] = 0.0;
  this->ModelBounds[2] = 0.0;
  this->ModelBounds[3] = 0.0;
  this->ModelBounds[4] = 0.0;
  this->ModelBounds[5] = 0.0;
  this->AdjustDistance = 0.10;

  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1.0;

  this->DensityEstimate = SVTK_DENSITY_ESTIMATE_RELATIVE_RADIUS;
  this->DensityForm = SVTK_DENSITY_FORM_NPTS;

  this->Radius = 1.0;
  this->RelativeRadius = 1.0;

  this->ScalarWeighting = false;

  this->ComputeGradient = false;

  this->Locator = svtkStaticPointLocator::New();
}

//----------------------------------------------------------------------------
svtkPointDensityFilter::~svtkPointDensityFilter()
{
  this->Locator->UnRegister(this);
  this->Locator = nullptr;
}

//----------------------------------------------------------------------------
int svtkPointDensityFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  return 1;
}

//----------------------------------------------------------------------------
int svtkPointDensityFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
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

//----------------------------------------------------------------------------
// Compute the size of the sample bounding box automatically from the
// input data.
void svtkPointDensityFilter::ComputeModelBounds(
  svtkDataSet* input, svtkImageData* output, svtkInformation* outInfo)
{
  const double* bounds;
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

  // Adjust bounds so model fits strictly inside (only if not set previously)
  if (adjustBounds)
  {
    double c, l;
    for (i = 0; i < 3; i++)
    {
      l = (1.0 + this->AdjustDistance) * (bounds[2 * i + 1] - bounds[2 * i]) / 2.0;
      c = (bounds[2 * i + 1] + bounds[2 * i]) / 2.0;
      this->ModelBounds[2 * i] = c - l;
      this->ModelBounds[2 * i + 1] = c + l;
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
}

//----------------------------------------------------------------------------
// Set the dimensions of the sampling volume
void svtkPointDensityFilter::SetSampleDimensions(int i, int j, int k)
{
  int dim[3];

  dim[0] = i;
  dim[1] = j;
  dim[2] = k;

  this->SetSampleDimensions(dim);
}

//----------------------------------------------------------------------------
void svtkPointDensityFilter::SetSampleDimensions(int dim[3])
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
// Produce the output data
int svtkPointDensityFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* output = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

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

  // Configure the output
  output->SetExtent(outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  output->AllocateScalars(outInfo);
  int* extent = this->GetExecutive()->GetOutputInformation(0)->Get(
    svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  // Configure the output
  output->SetDimensions(this->GetSampleDimensions());
  this->ComputeModelBounds(input, output, outInfo);

  //  Make sure points are available
  svtkIdType npts = input->GetNumberOfPoints();
  if (npts == 0)
  {
    svtkWarningMacro(<< "No POINTS input!!");
    return 1;
  }

  // Algorithm proper
  // Start by building the locator.
  if (!this->Locator)
  {
    svtkErrorMacro(<< "Point locator required\n");
    return 0;
  }
  this->Locator->SetDataSet(input);
  this->Locator->BuildLocator();

  // Determine the appropriate radius
  double radius;
  if (this->DensityEstimate == SVTK_DENSITY_ESTIMATE_FIXED_RADIUS)
  {
    radius = this->Radius;
  }
  else // SVTK_DENSITY_ESTIMATE_RELATIVE_RADIUS
  {
    radius = this->RelativeRadius * svtkMath::Norm(this->Spacing);
  }

  // If weighting points
  svtkDataArray* weights = this->GetInputArrayToProcess(0, inputVector);
  void* w = nullptr;
  if (weights && this->ScalarWeighting)
  {
    w = weights->GetVoidPointer(0);
  }

  // Grab the density array and process it.
  output->AllocateScalars(outInfo);
  svtkDataArray* density = output->GetPointData()->GetScalars();
  float* d = static_cast<float*>(output->GetArrayPointerForExtent(density, extent));

  int dims[3];
  double origin[3], spacing[3];
  output->GetDimensions(dims);
  output->GetOrigin(origin);
  output->GetSpacing(spacing);
  if (!w)
  {
    ComputePointDensity::Execute(this, dims, origin, spacing, d, radius, this->DensityForm);
  }
  else
  {
    switch (weights->GetDataType())
    {
      svtkTemplateMacro(ComputeWeightedDensity<SVTK_TT>::Execute(
        this, (SVTK_TT*)w, dims, origin, spacing, d, radius, this->DensityForm));
    }
  }

  // If the gradient is requested, compute the vector gradient and magnitude.
  // Also compute the classification of the gradient value.
  if (this->ComputeGradient)
  {
    // Allocate space
    svtkIdType num = density->GetNumberOfTuples();

    svtkFloatArray* gradients = svtkFloatArray::New();
    gradients->SetNumberOfComponents(3);
    gradients->SetNumberOfTuples(num);
    gradients->SetName("Gradient");
    output->GetPointData()->AddArray(gradients);
    float* grad = static_cast<float*>(gradients->GetVoidPointer(0));
    gradients->Delete();

    svtkFloatArray* magnitude = svtkFloatArray::New();
    magnitude->SetNumberOfComponents(1);
    magnitude->SetNumberOfTuples(num);
    magnitude->SetName("Gradient Magnitude");
    output->GetPointData()->AddArray(magnitude);
    float* mag = static_cast<float*>(magnitude->GetVoidPointer(0));
    magnitude->Delete();

    svtkUnsignedCharArray* fclassification = svtkUnsignedCharArray::New();
    fclassification->SetNumberOfComponents(1);
    fclassification->SetNumberOfTuples(num);
    fclassification->SetName("Classification");
    output->GetPointData()->AddArray(fclassification);
    unsigned char* fclass = static_cast<unsigned char*>(fclassification->GetVoidPointer(0));
    fclassification->Delete();

    // Thread the computation over slices
    ComputeGradients::Execute(dims, origin, spacing, d, grad, mag, fclass);
  }

  return 1;
}

//----------------------------------------------------------------------------
const char* svtkPointDensityFilter::GetDensityEstimateAsString()
{
  if (this->DensityEstimate == SVTK_DENSITY_ESTIMATE_FIXED_RADIUS)
  {
    return "Fixed Radius";
  }
  else // if ( this->DensityEstimate == SVTK_DENSITY_ESTIMATE_RELATIVE_RADIUS )
  {
    return "Relative Radius";
  }
}

//----------------------------------------------------------------------------
const char* svtkPointDensityFilter::GetDensityFormAsString()
{
  if (this->DensityForm == SVTK_DENSITY_FORM_VOLUME_NORM)
  {
    return "Volume Norm";
  }
  else // if ( this->DensityForm == SVTK_DENSITY_FORM_NPTS )
  {
    return "Number of Points";
  }
}

//----------------------------------------------------------------------------
void svtkPointDensityFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Sample Dimensions: (" << this->SampleDimensions[0] << ", "
     << this->SampleDimensions[1] << ", " << this->SampleDimensions[2] << ")\n";

  os << indent << "ModelBounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->ModelBounds[0] << ", " << this->ModelBounds[1] << ")\n";
  os << indent << "  Ymin,Ymax: (" << this->ModelBounds[2] << ", " << this->ModelBounds[3] << ")\n";
  os << indent << "  Zmin,Zmax: (" << this->ModelBounds[4] << ", " << this->ModelBounds[5] << ")\n";

  os << indent << "AdjustDistance: " << this->AdjustDistance << "\n";

  os << indent << "Density Estimate: " << this->GetDensityEstimateAsString() << "\n";

  os << indent << "Density Form: " << this->GetDensityFormAsString() << "\n";

  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Relative Radius: " << this->RelativeRadius << "\n";

  os << indent << "Scalar Weighting: " << (this->ScalarWeighting ? "On\n" : "Off\n");

  os << indent << "Compute Gradient: " << (this->ComputeGradient ? "On\n" : "Off\n");

  os << indent << "Locator: " << this->Locator << "\n";
}
