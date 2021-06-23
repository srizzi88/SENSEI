/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageReslice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageReslice.h"

#include "svtkGarbageCollector.h"
#include "svtkImageData.h"
#include "svtkImageInterpolator.h"
#include "svtkImagePointDataIterator.h"
#include "svtkImageStencilData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkIntArray.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTransform.h"

#include "svtkImageInterpolatorInternals.h"

#include "svtkTemplateAliasMacro.h"
// turn off 64-bit ints when templating over all types
#undef SVTK_USE_INT64
#define SVTK_USE_INT64 0
#undef SVTK_USE_UINT64
#define SVTK_USE_UINT64 0

#include <cfloat>
#include <climits>
#include <cmath>
#include <cstdint>

svtkStandardNewMacro(svtkImageReslice);
svtkCxxSetObjectMacro(svtkImageReslice, InformationInput, svtkImageData);
svtkCxxSetObjectMacro(svtkImageReslice, ResliceAxes, svtkMatrix4x4);
svtkCxxSetObjectMacro(svtkImageReslice, Interpolator, svtkAbstractImageInterpolator);
svtkCxxSetObjectMacro(svtkImageReslice, ResliceTransform, svtkAbstractTransform);

//--------------------------------------------------------------------------
// typedef for pixel converter method
typedef void (svtkImageReslice::*svtkImageResliceConvertScalarsType)(void* outPtr, void* inPtr,
  int inputType, int inNumComponents, int count, int idX, int idY, int idZ, int threadId);

// typedef for the floating point type used by the code
typedef double svtkImageResliceFloatingPointType;

//----------------------------------------------------------------------------
svtkImageReslice::svtkImageReslice()
{
  // if nullptr, the main Input is used
  this->InformationInput = nullptr;
  this->TransformInputSampling = 1;
  this->AutoCropOutput = 0;
  this->OutputDimensionality = 3;
  this->ComputeOutputSpacing = 1;
  this->ComputeOutputOrigin = 1;
  this->ComputeOutputExtent = 1;

  // flag to use default Spacing
  this->OutputSpacing[0] = 1.0;
  this->OutputSpacing[1] = 1.0;
  this->OutputSpacing[2] = 1.0;

  // ditto
  this->OutputOrigin[0] = 0.0;
  this->OutputOrigin[1] = 0.0;
  this->OutputOrigin[2] = 0.0;

  // ditto
  this->OutputExtent[0] = 0;
  this->OutputExtent[2] = 0;
  this->OutputExtent[4] = 0;

  this->OutputExtent[1] = 0;
  this->OutputExtent[3] = 0;
  this->OutputExtent[5] = 0;

  this->OutputScalarType = -1;

  this->Wrap = 0;   // don't wrap
  this->Mirror = 0; // don't mirror
  this->Border = 1; // apply a border
  this->BorderThickness = 0.5;
  this->InterpolationMode = SVTK_RESLICE_NEAREST; // no interpolation

  this->SlabMode = SVTK_IMAGE_SLAB_MEAN;
  this->SlabNumberOfSlices = 1;
  this->SlabTrapezoidIntegration = 0;
  this->SlabSliceSpacingFraction = 1.0;

  this->Optimization = 1; // turn off when you're paranoid

  // for rescaling the data
  this->ScalarShift = 0.0;
  this->ScalarScale = 1.0;

  // default black background
  this->BackgroundColor[0] = 0;
  this->BackgroundColor[1] = 0;
  this->BackgroundColor[2] = 0;
  this->BackgroundColor[3] = 0;

  // default reslice axes are x, y, z
  this->ResliceAxesDirectionCosines[0] = 1.0;
  this->ResliceAxesDirectionCosines[1] = 0.0;
  this->ResliceAxesDirectionCosines[2] = 0.0;
  this->ResliceAxesDirectionCosines[3] = 0.0;
  this->ResliceAxesDirectionCosines[4] = 1.0;
  this->ResliceAxesDirectionCosines[5] = 0.0;
  this->ResliceAxesDirectionCosines[6] = 0.0;
  this->ResliceAxesDirectionCosines[7] = 0.0;
  this->ResliceAxesDirectionCosines[8] = 1.0;

  // default (0,0,0) axes origin
  this->ResliceAxesOrigin[0] = 0.0;
  this->ResliceAxesOrigin[1] = 0.0;
  this->ResliceAxesOrigin[2] = 0.0;

  // axes and transform are identity if set to nullptr
  this->ResliceAxes = nullptr;
  this->ResliceTransform = nullptr;
  this->Interpolator = nullptr;

  // cache a matrix that converts output voxel indices -> input voxel indices
  this->IndexMatrix = nullptr;
  this->OptimizedTransform = nullptr;

  // set to zero when we completely missed the input extent
  this->HitInputExtent = 1;

  // set to true if PermuteExecute fast path will be used
  this->UsePermuteExecute = 0;

  // set in subclasses that convert the scalars after they are interpolated
  this->HasConvertScalars = 0;

  // the output stencil
  this->GenerateStencilOutput = 0;

  // There is an optional second input (the stencil input)
  this->SetNumberOfInputPorts(2);
  // There is an optional second output (the stencil output)
  this->SetNumberOfOutputPorts(2);

  // Create a stencil output (empty for now)
  svtkImageStencilData* stencil = svtkImageStencilData::New();
  this->GetExecutive()->SetOutputData(1, stencil);
  stencil->ReleaseData();
  stencil->Delete();
}

//----------------------------------------------------------------------------
svtkImageReslice::~svtkImageReslice()
{
  this->SetResliceTransform(nullptr);
  this->SetResliceAxes(nullptr);
  if (this->IndexMatrix)
  {
    this->IndexMatrix->Delete();
  }
  if (this->OptimizedTransform)
  {
    this->OptimizedTransform->Delete();
  }
  this->SetInformationInput(nullptr);
  this->SetInterpolator(nullptr);
}

//----------------------------------------------------------------------------
void svtkImageReslice::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ResliceAxes: " << this->ResliceAxes << "\n";
  if (this->ResliceAxes)
  {
    this->ResliceAxes->PrintSelf(os, indent.GetNextIndent());
  }
  this->GetResliceAxesDirectionCosines(this->ResliceAxesDirectionCosines);
  os << indent << "ResliceAxesDirectionCosines: " << this->ResliceAxesDirectionCosines[0] << " "
     << this->ResliceAxesDirectionCosines[1] << " " << this->ResliceAxesDirectionCosines[2] << "\n";
  os << indent << "                             " << this->ResliceAxesDirectionCosines[3] << " "
     << this->ResliceAxesDirectionCosines[4] << " " << this->ResliceAxesDirectionCosines[5] << "\n";
  os << indent << "                             " << this->ResliceAxesDirectionCosines[6] << " "
     << this->ResliceAxesDirectionCosines[7] << " " << this->ResliceAxesDirectionCosines[8] << "\n";
  this->GetResliceAxesOrigin(this->ResliceAxesOrigin);
  os << indent << "ResliceAxesOrigin: " << this->ResliceAxesOrigin[0] << " "
     << this->ResliceAxesOrigin[1] << " " << this->ResliceAxesOrigin[2] << "\n";
  os << indent << "ResliceTransform: " << this->ResliceTransform << "\n";
  if (this->ResliceTransform)
  {
    this->ResliceTransform->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "Interpolator: " << this->Interpolator << "\n";
  os << indent << "InformationInput: " << this->InformationInput << "\n";
  os << indent << "TransformInputSampling: " << (this->TransformInputSampling ? "On\n" : "Off\n");
  os << indent << "AutoCropOutput: " << (this->AutoCropOutput ? "On\n" : "Off\n");
  os << indent << "OutputSpacing: " << this->OutputSpacing[0] << " " << this->OutputSpacing[1]
     << " " << this->OutputSpacing[2] << "\n";
  os << indent << "OutputOrigin: " << this->OutputOrigin[0] << " " << this->OutputOrigin[1] << " "
     << this->OutputOrigin[2] << "\n";
  os << indent << "OutputExtent: " << this->OutputExtent[0] << " " << this->OutputExtent[1] << " "
     << this->OutputExtent[2] << " " << this->OutputExtent[3] << " " << this->OutputExtent[4] << " "
     << this->OutputExtent[5] << "\n";
  os << indent << "OutputDimensionality: " << this->OutputDimensionality << "\n";
  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "Wrap: " << (this->Wrap ? "On\n" : "Off\n");
  os << indent << "Mirror: " << (this->Mirror ? "On\n" : "Off\n");
  os << indent << "Border: " << (this->Border ? "On\n" : "Off\n");
  os << indent << "BorderThickness: " << this->BorderThickness << "\n";
  os << indent << "InterpolationMode: " << this->GetInterpolationModeAsString() << "\n";
  os << indent << "SlabMode: " << this->GetSlabModeAsString() << "\n";
  os << indent << "SlabNumberOfSlices: " << this->SlabNumberOfSlices << "\n";
  os << indent
     << "SlabTrapezoidIntegration: " << (this->SlabTrapezoidIntegration ? "On\n" : "Off\n");
  os << indent << "SlabSliceSpacingFraction: " << this->SlabSliceSpacingFraction << "\n";
  os << indent << "Optimization: " << (this->Optimization ? "On\n" : "Off\n");
  os << indent << "ScalarShift: " << this->ScalarShift << "\n";
  os << indent << "ScalarScale: " << this->ScalarScale << "\n";
  os << indent << "BackgroundColor: " << this->BackgroundColor[0] << " " << this->BackgroundColor[1]
     << " " << this->BackgroundColor[2] << " " << this->BackgroundColor[3] << "\n";
  os << indent << "BackgroundLevel: " << this->BackgroundColor[0] << "\n";
  os << indent << "Stencil: " << this->GetStencil() << "\n";
  os << indent << "GenerateStencilOutput: " << (this->GenerateStencilOutput ? "On\n" : "Off\n");
  os << indent << "StencilOutput: " << this->GetStencilOutput() << "\n";
}

//----------------------------------------------------------------------------
void svtkImageReslice::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  svtkGarbageCollectorReport(collector, this->InformationInput, "InformationInput");
}

//----------------------------------------------------------------------------
void svtkImageReslice::SetOutputSpacing(double x, double y, double z)
{
  double* s = this->OutputSpacing;
  if (s[0] != x || s[1] != y || s[2] != z)
  {
    this->OutputSpacing[0] = x;
    this->OutputSpacing[1] = y;
    this->OutputSpacing[2] = z;
    this->Modified();
  }
  else if (this->ComputeOutputSpacing)
  {
    this->Modified();
  }
  this->ComputeOutputSpacing = 0;
}

//----------------------------------------------------------------------------
void svtkImageReslice::SetOutputSpacingToDefault()
{
  if (!this->ComputeOutputSpacing)
  {
    this->OutputSpacing[0] = 1.0;
    this->OutputSpacing[1] = 1.0;
    this->OutputSpacing[2] = 1.0;
    this->ComputeOutputSpacing = 1;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkImageReslice::SetOutputOrigin(double x, double y, double z)
{
  double* o = this->OutputOrigin;
  if (o[0] != x || o[1] != y || o[2] != z)
  {
    this->OutputOrigin[0] = x;
    this->OutputOrigin[1] = y;
    this->OutputOrigin[2] = z;
    this->Modified();
  }
  else if (this->ComputeOutputOrigin)
  {
    this->Modified();
  }
  this->ComputeOutputOrigin = 0;
}

//----------------------------------------------------------------------------
void svtkImageReslice::SetOutputOriginToDefault()
{
  if (!this->ComputeOutputOrigin)
  {
    this->OutputOrigin[0] = 0.0;
    this->OutputOrigin[1] = 0.0;
    this->OutputOrigin[2] = 0.0;
    this->ComputeOutputOrigin = 1;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkImageReslice::SetOutputExtent(int a, int b, int c, int d, int e, int f)
{
  int* extent = this->OutputExtent;
  if (extent[0] != a || extent[1] != b || extent[2] != c || extent[3] != d || extent[4] != e ||
    extent[5] != f)
  {
    this->OutputExtent[0] = a;
    this->OutputExtent[1] = b;
    this->OutputExtent[2] = c;
    this->OutputExtent[3] = d;
    this->OutputExtent[4] = e;
    this->OutputExtent[5] = f;
    this->Modified();
  }
  else if (this->ComputeOutputExtent)
  {
    this->Modified();
  }
  this->ComputeOutputExtent = 0;
}

//----------------------------------------------------------------------------
void svtkImageReslice::SetOutputExtentToDefault()
{
  if (!this->ComputeOutputExtent)
  {
    this->OutputExtent[0] = 0;
    this->OutputExtent[2] = 0;
    this->OutputExtent[4] = 0;
    this->OutputExtent[1] = 0;
    this->OutputExtent[3] = 0;
    this->OutputExtent[5] = 0;
    this->ComputeOutputExtent = 1;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const char* svtkImageReslice::GetInterpolationModeAsString()
{
  switch (this->InterpolationMode)
  {
    case SVTK_RESLICE_NEAREST:
      return "NearestNeighbor";
    case SVTK_RESLICE_LINEAR:
      return "Linear";
    case SVTK_RESLICE_CUBIC:
      return "Cubic";
  }
  return "";
}

//----------------------------------------------------------------------------
const char* svtkImageReslice::GetSlabModeAsString()
{
  switch (this->SlabMode)
  {
    case SVTK_IMAGE_SLAB_MIN:
      return "Min";
    case SVTK_IMAGE_SLAB_MAX:
      return "Max";
    case SVTK_IMAGE_SLAB_MEAN:
      return "Mean";
    case SVTK_IMAGE_SLAB_SUM:
      return "Sum";
  }
  return "";
}

//----------------------------------------------------------------------------
void svtkImageReslice::SetStencilData(svtkImageStencilData* stencil)
{
  this->SetInputData(1, stencil);
}

//----------------------------------------------------------------------------
svtkImageStencilData* svtkImageReslice::GetStencil()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }
  return svtkImageStencilData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
void svtkImageReslice::SetStencilOutput(svtkImageStencilData* output)
{
  this->GetExecutive()->SetOutputData(1, output);
}

//----------------------------------------------------------------------------
svtkImageStencilData* svtkImageReslice::GetStencilOutput()
{
  if (this->GetNumberOfOutputPorts() < 2)
  {
    return nullptr;
  }

  return svtkImageStencilData::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//----------------------------------------------------------------------------
void svtkImageReslice::SetResliceAxesDirectionCosines(
  double x0, double x1, double x2, double y0, double y1, double y2, double z0, double z1, double z2)
{
  if (!this->ResliceAxes)
  {
    // consistent registers/unregisters
    this->SetResliceAxes(svtkMatrix4x4::New());
    this->ResliceAxes->Delete();
    this->Modified();
  }
  this->ResliceAxes->SetElement(0, 0, x0);
  this->ResliceAxes->SetElement(1, 0, x1);
  this->ResliceAxes->SetElement(2, 0, x2);
  this->ResliceAxes->SetElement(3, 0, 0);
  this->ResliceAxes->SetElement(0, 1, y0);
  this->ResliceAxes->SetElement(1, 1, y1);
  this->ResliceAxes->SetElement(2, 1, y2);
  this->ResliceAxes->SetElement(3, 1, 0);
  this->ResliceAxes->SetElement(0, 2, z0);
  this->ResliceAxes->SetElement(1, 2, z1);
  this->ResliceAxes->SetElement(2, 2, z2);
  this->ResliceAxes->SetElement(3, 2, 0);
}

//----------------------------------------------------------------------------
void svtkImageReslice::GetResliceAxesDirectionCosines(
  double xdircos[3], double ydircos[3], double zdircos[3])
{
  if (!this->ResliceAxes)
  {
    xdircos[0] = ydircos[1] = zdircos[2] = 1;
    xdircos[1] = ydircos[2] = zdircos[0] = 0;
    xdircos[2] = ydircos[0] = zdircos[1] = 0;
    return;
  }

  for (int i = 0; i < 3; i++)
  {
    xdircos[i] = this->ResliceAxes->GetElement(i, 0);
    ydircos[i] = this->ResliceAxes->GetElement(i, 1);
    zdircos[i] = this->ResliceAxes->GetElement(i, 2);
  }
}

//----------------------------------------------------------------------------
void svtkImageReslice::SetResliceAxesOrigin(double x, double y, double z)
{
  if (!this->ResliceAxes)
  {
    // consistent registers/unregisters
    this->SetResliceAxes(svtkMatrix4x4::New());
    this->ResliceAxes->Delete();
    this->Modified();
  }

  this->ResliceAxes->SetElement(0, 3, x);
  this->ResliceAxes->SetElement(1, 3, y);
  this->ResliceAxes->SetElement(2, 3, z);
  this->ResliceAxes->SetElement(3, 3, 1);
}

//----------------------------------------------------------------------------
void svtkImageReslice::GetResliceAxesOrigin(double origin[3])
{
  if (!this->ResliceAxes)
  {
    origin[0] = origin[1] = origin[2] = 0;
    return;
  }

  for (int i = 0; i < 3; i++)
  {
    origin[i] = this->ResliceAxes->GetElement(i, 3);
  }
}

//----------------------------------------------------------------------------
svtkAbstractImageInterpolator* svtkImageReslice::GetInterpolator()
{
  if (this->Interpolator == nullptr)
  {
    this->Interpolator = svtkImageInterpolator::New();
  }

  return this->Interpolator;
}

//----------------------------------------------------------------------------
// Account for the MTime of the transform and its matrix when determining
// the MTime of the filter
svtkMTimeType svtkImageReslice::GetMTime()
{
  svtkMTimeType mTime = this->Superclass::GetMTime();
  svtkMTimeType time;

  if (this->ResliceTransform != nullptr)
  {
    time = this->ResliceTransform->GetMTime();
    mTime = (time > mTime ? time : mTime);
    if (this->ResliceTransform->IsA("svtkHomogeneousTransform"))
    { // this is for people who directly modify the transform matrix
      time =
        (static_cast<svtkHomogeneousTransform*>(this->ResliceTransform))->GetMatrix()->GetMTime();
      mTime = (time > mTime ? time : mTime);
    }
  }
  if (this->ResliceAxes != nullptr)
  {
    time = this->ResliceAxes->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  if (this->Interpolator != nullptr)
  {
    time = this->Interpolator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
int svtkImageReslice::ConvertScalarInfo(int& svtkNotUsed(scalarType), int& svtkNotUsed(numComponents))
{
  return 1;
}

//----------------------------------------------------------------------------
void svtkImageReslice::ConvertScalars(void* svtkNotUsed(inPtr), void* svtkNotUsed(outPtr),
  int svtkNotUsed(inputType), int svtkNotUsed(inputComponents), int svtkNotUsed(count),
  int svtkNotUsed(idX), int svtkNotUsed(idY), int svtkNotUsed(idZ), int svtkNotUsed(threadId))
{
}

//----------------------------------------------------------------------------
int svtkImageReslice::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int inExt[6], outExt[6];
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);
  this->HitInputExtent = 1;

  if (this->ResliceTransform)
  {
    this->ResliceTransform->Update();
    if (!this->ResliceTransform->IsA("svtkHomogeneousTransform"))
    { // update the whole input extent if the transform is nonlinear
      inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inExt);
      inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);
      return 1;
    }
  }

  bool wrap = (this->Wrap || this->Mirror);

  double xAxis[4], yAxis[4], zAxis[4], origin[4];

  svtkMatrix4x4* matrix = this->GetIndexMatrix(inInfo, outInfo);

  // convert matrix from world coordinates to pixel indices
  for (int i = 0; i < 4; i++)
  {
    xAxis[i] = matrix->GetElement(i, 0);
    yAxis[i] = matrix->GetElement(i, 1);
    zAxis[i] = matrix->GetElement(i, 2);
    origin[i] = matrix->GetElement(i, 3);
  }

  for (int i = 0; i < 3; i++)
  {
    inExt[2 * i] = SVTK_INT_MAX;
    inExt[2 * i + 1] = SVTK_INT_MIN;
  }

  if (this->SlabNumberOfSlices > 1)
  {
    outExt[4] -= (this->SlabNumberOfSlices + 1) / 2;
    outExt[5] += (this->SlabNumberOfSlices + 1) / 2;
  }

  // set the extent according to the interpolation kernel size
  svtkAbstractImageInterpolator* interpolator = this->GetInterpolator();
  double* elements = *matrix->Element;
  elements = ((this->OptimizedTransform == nullptr) ? elements : nullptr);
  int supportSize[3];
  interpolator->ComputeSupportSize(elements, supportSize);

  // check the coordinates of the 8 corners of the output extent
  // (this must be done exactly the same as the calculation in
  // svtkImageResliceExecute)
  for (int jj = 0; jj < 8; jj++)
  {
    // get output coords
    int idX = outExt[jj % 2];
    int idY = outExt[2 + (jj / 2) % 2];
    int idZ = outExt[4 + (jj / 4) % 2];

    double inPoint0[4];
    inPoint0[0] = origin[0] + idZ * zAxis[0]; // incremental transform
    inPoint0[1] = origin[1] + idZ * zAxis[1];
    inPoint0[2] = origin[2] + idZ * zAxis[2];
    inPoint0[3] = origin[3] + idZ * zAxis[3];

    double inPoint1[4];
    inPoint1[0] = inPoint0[0] + idY * yAxis[0]; // incremental transform
    inPoint1[1] = inPoint0[1] + idY * yAxis[1];
    inPoint1[2] = inPoint0[2] + idY * yAxis[2];
    inPoint1[3] = inPoint0[3] + idY * yAxis[3];

    double point[4];
    point[0] = inPoint1[0] + idX * xAxis[0];
    point[1] = inPoint1[1] + idX * xAxis[1];
    point[2] = inPoint1[2] + idX * xAxis[2];
    point[3] = inPoint1[3] + idX * xAxis[3];

    if (point[3] != 1.0)
    {
      double f = 1 / point[3];
      point[0] *= f;
      point[1] *= f;
      point[2] *= f;
    }

    for (int j = 0; j < 3; j++)
    {
      int kernelSize = supportSize[j];
      int extra = (kernelSize + 1) / 2 - 1;

      // most kernels have even size
      if ((kernelSize & 1) == 0)
      {
        double f;
        int k = svtkInterpolationMath::Floor(point[j], f);
        if (k - extra < inExt[2 * j])
        {
          inExt[2 * j] = k - extra;
        }
        k += (f != 0);
        if (k + extra > inExt[2 * j + 1])
        {
          inExt[2 * j + 1] = k + extra;
        }
      }
      // else is for kernels with odd size
      else
      {
        int k = svtkInterpolationMath::Round(point[j]);
        if (k < inExt[2 * j])
        {
          inExt[2 * j] = k - extra;
        }
        if (k > inExt[2 * j + 1])
        {
          inExt[2 * j + 1] = k + extra;
        }
      }
    }
  }

  // Clip to whole extent, make sure we hit the extent
  int wholeExtent[6];
  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);

  for (int k = 0; k < 3; k++)
  {
    if (inExt[2 * k] < wholeExtent[2 * k])
    {
      inExt[2 * k] = wholeExtent[2 * k];
      if (wrap)
      {
        inExt[2 * k + 1] = wholeExtent[2 * k + 1];
      }
      else if (inExt[2 * k + 1] < wholeExtent[2 * k])
      {
        // didn't hit any of the input extent
        inExt[2 * k + 1] = wholeExtent[2 * k];
        this->HitInputExtent = 0;
      }
    }
    if (inExt[2 * k + 1] > wholeExtent[2 * k + 1])
    {
      inExt[2 * k + 1] = wholeExtent[2 * k + 1];
      if (wrap)
      {
        inExt[2 * k] = wholeExtent[2 * k];
      }
      else if (inExt[2 * k] > wholeExtent[2 * k + 1])
      {
        // didn't hit any of the input extent
        inExt[2 * k] = wholeExtent[2 * k + 1];
        // finally, check for null input extent
        if (inExt[2 * k] < wholeExtent[2 * k])
        {
          inExt[2 * k] = wholeExtent[2 * k];
        }
        this->HitInputExtent = 0;
      }
    }
  }

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  // need to set the stencil update extent to the output extent
  if (this->GetNumberOfInputConnections(1) > 0)
  {
    svtkInformation* stencilInfo = inputVector[1]->GetInformationObject(0);
    stencilInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt, 6);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageReslice::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 1)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageStencilData");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  else
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkImageReslice::FillOutputPortInformation(int port, svtkInformation* info)
{
  if (port == 1)
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageStencilData");
  }
  else
  {
    info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkImageData");
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkImageReslice::AllocateOutputData(
  svtkImageData* output, svtkInformation* outInfo, int* uExtent)
{
  // set the extent to be the update extent
  output->SetExtent(uExtent);
  output->AllocateScalars(outInfo);

  svtkImageStencilData* stencil = this->GetStencilOutput();
  if (stencil && this->GenerateStencilOutput)
  {
    stencil->SetExtent(uExtent);
    stencil->AllocateExtents();
  }
}

//----------------------------------------------------------------------------
svtkImageData* svtkImageReslice::AllocateOutputData(svtkDataObject* output, svtkInformation* outInfo)
{
  return this->Superclass::AllocateOutputData(output, outInfo);
}

//----------------------------------------------------------------------------
void svtkImageReslice::GetAutoCroppedOutputBounds(svtkInformation* inInfo, double bounds[6])
{
  int i, j;
  double inSpacing[3], inOrigin[3];
  int inWholeExt[6];
  double f;
  double point[4];

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inWholeExt);
  inInfo->Get(svtkDataObject::SPACING(), inSpacing);
  inInfo->Get(svtkDataObject::ORIGIN(), inOrigin);

  svtkMatrix4x4* matrix = svtkMatrix4x4::New();
  if (this->ResliceAxes)
  {
    svtkMatrix4x4::Invert(this->ResliceAxes, matrix);
  }
  svtkAbstractTransform* transform = nullptr;
  if (this->ResliceTransform)
  {
    transform = this->ResliceTransform->GetInverse();
  }

  for (i = 0; i < 3; i++)
  {
    bounds[2 * i] = SVTK_DOUBLE_MAX;
    bounds[2 * i + 1] = -SVTK_DOUBLE_MAX;
  }

  for (i = 0; i < 8; i++)
  {
    point[0] = inOrigin[0] + inWholeExt[i % 2] * inSpacing[0];
    point[1] = inOrigin[1] + inWholeExt[2 + (i / 2) % 2] * inSpacing[1];
    point[2] = inOrigin[2] + inWholeExt[4 + (i / 4) % 2] * inSpacing[2];
    point[3] = 1.0;

    if (this->ResliceTransform)
    {
      transform->TransformPoint(point, point);
    }
    matrix->MultiplyPoint(point, point);

    f = 1.0 / point[3];
    point[0] *= f;
    point[1] *= f;
    point[2] *= f;

    for (j = 0; j < 3; j++)
    {
      if (point[j] > bounds[2 * j + 1])
      {
        bounds[2 * j + 1] = point[j];
      }
      if (point[j] < bounds[2 * j])
      {
        bounds[2 * j] = point[j];
      }
    }
  }

  matrix->Delete();
}

//----------------------------------------------------------------------------
namespace
{
//----------------------------------------------------------------------------
// check a matrix to ensure that it is a permutation+scale+translation
// matrix

int svtkIsPermutationMatrix(svtkMatrix4x4* matrix)
{
  for (int i = 0; i < 3; i++)
  {
    if (matrix->GetElement(3, i) != 0)
    {
      return 0;
    }
  }
  if (matrix->GetElement(3, 3) != 1)
  {
    return 0;
  }
  for (int j = 0; j < 3; j++)
  {
    int k = 0;
    for (int i = 0; i < 3; i++)
    {
      if (matrix->GetElement(i, j) != 0)
      {
        k++;
      }
    }
    if (k != 1)
    {
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
// Check to see if we can do nearest-neighbor instead of linear or cubic.
// This check only works on permutation+scale+translation matrices.
int svtkCanUseNearestNeighbor(svtkMatrix4x4* matrix, int outExt[6])
{
  // loop through dimensions
  for (int i = 0; i < 3; i++)
  {
    int j;
    for (j = 0; j < 3; j++)
    {
      if (matrix->GetElement(i, j) != 0)
      {
        break;
      }
    }
    if (j >= 3)
    {
      assert(0);
      return 0;
    }
    double x = matrix->GetElement(i, j);
    double y = matrix->GetElement(i, 3);
    if (outExt[2 * j] == outExt[2 * j + 1])
    {
      y += x * outExt[2 * i];
      x = 0;
    }
    double fx, fy;
    svtkInterpolationMath::Floor(x, fx);
    svtkInterpolationMath::Floor(y, fy);
    if (fx != 0 || fy != 0)
    {
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
// check a matrix to see whether it is the identity matrix

int svtkIsIdentityMatrix(svtkMatrix4x4* matrix)
{
  static double identity[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
  int i, j;

  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 4; j++)
    {
      if (matrix->GetElement(i, j) != identity[4 * i + j])
      {
        return 0;
      }
    }
  }
  return 1;
}

} // end anonymous namespace

//----------------------------------------------------------------------------
int svtkImageReslice::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int i, j;
  double inSpacing[3], inOrigin[3];
  int inWholeExt[6];
  double outSpacing[3], outOrigin[3];
  int outWholeExt[6];
  double maxBounds[6];

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  if (this->InformationInput)
  {
    this->InformationInput->GetExtent(inWholeExt);
    this->InformationInput->GetSpacing(inSpacing);
    this->InformationInput->GetOrigin(inOrigin);
  }
  else
  {
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inWholeExt);
    inInfo->Get(svtkDataObject::SPACING(), inSpacing);
    inInfo->Get(svtkDataObject::ORIGIN(), inOrigin);
  }

  // reslice axes matrix is identity by default
  double matrix[4][4];
  double imatrix[4][4];
  for (i = 0; i < 4; i++)
  {
    matrix[i][0] = matrix[i][1] = matrix[i][2] = matrix[i][3] = 0;
    matrix[i][i] = 1;
    imatrix[i][0] = imatrix[i][1] = imatrix[i][2] = imatrix[i][3] = 0;
    imatrix[i][i] = 1;
  }
  if (this->ResliceAxes)
  {
    svtkMatrix4x4::DeepCopy(*matrix, this->ResliceAxes);
    svtkMatrix4x4::Invert(*matrix, *imatrix);
  }

  if (this->AutoCropOutput)
  {
    this->GetAutoCroppedOutputBounds(inInfo, maxBounds);
  }

  // pass the center of the volume through the inverse of the
  // 3x3 direction cosines matrix
  double inCenter[3];
  for (i = 0; i < 3; i++)
  {
    inCenter[i] = inOrigin[i] + 0.5 * (inWholeExt[2 * i] + inWholeExt[2 * i + 1]) * inSpacing[i];
  }

  // the default spacing, extent and origin are the input spacing, extent
  // and origin,  transformed by the direction cosines of the ResliceAxes
  // if requested (note that the transformed output spacing will always
  // be positive)
  for (i = 0; i < 3; i++)
  {
    double s = 0; // default output spacing
    double d = 0; // default linear dimension
    double e = 0; // default extent start
    double c = 0; // transformed center-of-volume

    if (this->TransformInputSampling)
    {
      double r = 0.0;
      for (j = 0; j < 3; j++)
      {
        c += imatrix[i][j] * (inCenter[j] - matrix[j][3]);
        double tmp = matrix[j][i] * matrix[j][i];
        s += tmp * fabs(inSpacing[j]);
        d += tmp * (inWholeExt[2 * j + 1] - inWholeExt[2 * j]) * fabs(inSpacing[j]);
        e += tmp * inWholeExt[2 * j];
        r += tmp;
      }
      s /= r;
      d /= r * sqrt(r);
      e /= r;
    }
    else
    {
      c = inCenter[i];
      s = inSpacing[i];
      d = (inWholeExt[2 * i + 1] - inWholeExt[2 * i]) * s;
      e = inWholeExt[2 * i];
    }

    if (this->ComputeOutputSpacing)
    {
      outSpacing[i] = s;
    }
    else
    {
      outSpacing[i] = this->OutputSpacing[i];
    }

    if (i >= this->OutputDimensionality)
    {
      outWholeExt[2 * i] = 0;
      outWholeExt[2 * i + 1] = 0;
    }
    else if (this->ComputeOutputExtent)
    {
      if (this->AutoCropOutput)
      {
        d = maxBounds[2 * i + 1] - maxBounds[2 * i];
      }
      outWholeExt[2 * i] = svtkInterpolationMath::Round(e);
      outWholeExt[2 * i + 1] =
        svtkInterpolationMath::Round(outWholeExt[2 * i] + fabs(d / outSpacing[i]));
    }
    else
    {
      outWholeExt[2 * i] = this->OutputExtent[2 * i];
      outWholeExt[2 * i + 1] = this->OutputExtent[2 * i + 1];
    }

    if (i >= this->OutputDimensionality)
    {
      outOrigin[i] = 0;
    }
    else if (this->ComputeOutputOrigin)
    {
      if (this->AutoCropOutput)
      { // set origin so edge of extent is edge of bounds
        outOrigin[i] = maxBounds[2 * i] - outWholeExt[2 * i] * outSpacing[i];
      }
      else
      { // center new bounds over center of input bounds
        outOrigin[i] = c - 0.5 * (outWholeExt[2 * i] + outWholeExt[2 * i + 1]) * outSpacing[i];
      }
    }
    else
    {
      outOrigin[i] = this->OutputOrigin[i];
    }
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outWholeExt, 6);
  outInfo->Set(svtkDataObject::SPACING(), outSpacing, 3);
  outInfo->Set(svtkDataObject::ORIGIN(), outOrigin, 3);

  svtkInformation* outStencilInfo = outputVector->GetInformationObject(1);
  if (this->GenerateStencilOutput)
  {
    outStencilInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), outWholeExt, 6);
    outStencilInfo->Set(svtkDataObject::SPACING(), outSpacing, 3);
    outStencilInfo->Set(svtkDataObject::ORIGIN(), outOrigin, 3);
  }
  else if (outStencilInfo)
  {
    // If we are not generating stencil output, remove all meta-data
    // that the executives copy from the input by default
    outStencilInfo->Remove(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    outStencilInfo->Remove(svtkDataObject::SPACING());
    outStencilInfo->Remove(svtkDataObject::ORIGIN());
  }

  // get the interpolator
  svtkAbstractImageInterpolator* interpolator = this->GetInterpolator();

  // set the scalar information
  svtkInformation* inScalarInfo = svtkDataObject::GetActiveFieldInformation(
    inInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);

  int scalarType = -1;
  int numComponents = -1;

  if (inScalarInfo)
  {
    scalarType = inScalarInfo->Get(svtkDataObject::FIELD_ARRAY_TYPE());

    if (inScalarInfo->Has(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()))
    {
      numComponents = interpolator->ComputeNumberOfComponents(
        inScalarInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS()));
    }
  }

  if (this->HasConvertScalars)
  {
    this->ConvertScalarInfo(scalarType, numComponents);

    svtkDataObject::SetPointDataActiveScalarInfo(outInfo, scalarType, numComponents);
  }
  else
  {
    if (this->OutputScalarType > 0)
    {
      scalarType = this->OutputScalarType;
    }

    svtkDataObject::SetPointDataActiveScalarInfo(outInfo, scalarType, numComponents);
  }

  // create a matrix for structured coordinate conversion
  this->GetIndexMatrix(inInfo, outInfo);

  // check for possible optimizations
  int interpolationMode = this->InterpolationMode;
  this->UsePermuteExecute = 0;
  if (this->Optimization)
  {
    if (this->OptimizedTransform == nullptr && this->SlabSliceSpacingFraction == 1.0 &&
      interpolator->IsSeparable() && svtkIsPermutationMatrix(this->IndexMatrix))
    {
      this->UsePermuteExecute = 1;
      if (svtkCanUseNearestNeighbor(this->IndexMatrix, outWholeExt))
      {
        interpolationMode = SVTK_NEAREST_INTERPOLATION;
      }
    }
  }

  // set the interpolator information
  if (interpolator->IsA("svtkImageInterpolator"))
  {
    static_cast<svtkImageInterpolator*>(interpolator)->SetInterpolationMode(interpolationMode);
  }
  int borderMode = SVTK_IMAGE_BORDER_CLAMP;
  borderMode = (this->Wrap ? SVTK_IMAGE_BORDER_REPEAT : borderMode);
  borderMode = (this->Mirror ? SVTK_IMAGE_BORDER_MIRROR : borderMode);
  interpolator->SetBorderMode(borderMode);

  // set the tolerance according to the border mode, use infinite
  // (or at least very large) tolerance for wrap and mirror
  static double mintol = SVTK_INTERPOLATE_FLOOR_TOL;
  static double maxtol = 2.0 * SVTK_INT_MAX;
  double tol = (this->Border ? this->BorderThickness : 0.0);
  tol = ((borderMode == SVTK_IMAGE_BORDER_CLAMP) ? tol : maxtol);
  tol = ((tol > mintol) ? tol : mintol);
  interpolator->SetTolerance(tol);

  return 1;
}

//----------------------------------------------------------------------------
// rounding functions for each type, where 'F' is a floating-point type

namespace
{

#if (SVTK_USE_INT8 != 0)
template <class F>
inline void svtkInterpolateRound(F val, svtkTypeInt8& rnd)
{
  rnd = svtkInterpolationMath::Round(val);
}
#endif

#if (SVTK_USE_UINT8 != 0)
template <class F>
inline void svtkInterpolateRound(F val, svtkTypeUInt8& rnd)
{
  rnd = svtkInterpolationMath::Round(val);
}
#endif

#if (SVTK_USE_INT16 != 0)
template <class F>
inline void svtkInterpolateRound(F val, svtkTypeInt16& rnd)
{
  rnd = svtkInterpolationMath::Round(val);
}
#endif

#if (SVTK_USE_UINT16 != 0)
template <class F>
inline void svtkInterpolateRound(F val, svtkTypeUInt16& rnd)
{
  rnd = svtkInterpolationMath::Round(val);
}
#endif

#if (SVTK_USE_INT32 != 0)
template <class F>
inline void svtkInterpolateRound(F val, svtkTypeInt32& rnd)
{
  rnd = svtkInterpolationMath::Round(val);
}
#endif

#if (SVTK_USE_UINT32 != 0)
template <class F>
inline void svtkInterpolateRound(F val, svtkTypeUInt32& rnd)
{
  rnd = svtkInterpolationMath::Round(val);
}
#endif

#if (SVTK_USE_FLOAT32 != 0)
template <class F>
inline void svtkInterpolateRound(F val, svtkTypeFloat32& rnd)
{
  rnd = val;
}
#endif

#if (SVTK_USE_FLOAT64 != 0)
template <class F>
inline void svtkInterpolateRound(F val, svtkTypeFloat64& rnd)
{
  rnd = val;
}
#endif

//----------------------------------------------------------------------------
// clamping functions for each type

template <class F>
inline F svtkResliceClamp(F x, F xmin, F xmax)
{
  // do not change this code: it compiles into min/max opcodes
  x = (x > xmin ? x : xmin);
  x = (x < xmax ? x : xmax);
  return x;
}

#if (SVTK_USE_INT8 != 0)
template <class F>
inline void svtkResliceClamp(F val, svtkTypeInt8& clamp)
{
  static F minval = static_cast<F>(-128.0);
  static F maxval = static_cast<F>(127.0);
  val = svtkResliceClamp(val, minval, maxval);
  svtkInterpolateRound(val, clamp);
}
#endif

#if (SVTK_USE_UINT8 != 0)
template <class F>
inline void svtkResliceClamp(F val, svtkTypeUInt8& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(255.0);
  val = svtkResliceClamp(val, minval, maxval);
  svtkInterpolateRound(val, clamp);
}
#endif

#if (SVTK_USE_INT16 != 0)
template <class F>
inline void svtkResliceClamp(F val, svtkTypeInt16& clamp)
{
  static F minval = static_cast<F>(-32768.0);
  static F maxval = static_cast<F>(32767.0);
  val = svtkResliceClamp(val, minval, maxval);
  svtkInterpolateRound(val, clamp);
}
#endif

#if (SVTK_USE_UINT16 != 0)
template <class F>
inline void svtkResliceClamp(F val, svtkTypeUInt16& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(65535.0);
  val = svtkResliceClamp(val, minval, maxval);
  svtkInterpolateRound(val, clamp);
}
#endif

#if (SVTK_USE_INT32 != 0)
template <class F>
inline void svtkResliceClamp(F val, svtkTypeInt32& clamp)
{
  static F minval = static_cast<F>(-2147483648.0);
  static F maxval = static_cast<F>(2147483647.0);
  val = svtkResliceClamp(val, minval, maxval);
  svtkInterpolateRound(val, clamp);
}
#endif

#if (SVTK_USE_UINT32 != 0)
template <class F>
inline void svtkResliceClamp(F val, svtkTypeUInt32& clamp)
{
  static F minval = static_cast<F>(0);
  static F maxval = static_cast<F>(4294967295.0);
  val = svtkResliceClamp(val, minval, maxval);
  svtkInterpolateRound(val, clamp);
}
#endif

#if (SVTK_USE_FLOAT32 != 0)
template <class F>
inline void svtkResliceClamp(F val, svtkTypeFloat32& clamp)
{
  clamp = val;
}
#endif

#if (SVTK_USE_FLOAT64 != 0)
template <class F>
inline void svtkResliceClamp(F val, svtkTypeFloat64& clamp)
{
  clamp = val;
}
#endif

//----------------------------------------------------------------------------
// Convert from float to any type, with clamping or not.
template <class F, class T>
struct svtkImageResliceConversion
{
  static void Convert(void*& outPtr, const F* inPtr, int numscalars, int n);

  static void Clamp(void*& outPtr, const F* inPtr, int numscalars, int n);
};

template <class F, class T>
void svtkImageResliceConversion<F, T>::Convert(void*& outPtr0, const F* inPtr, int numscalars, int n)
{
  if (n > 0)
  {
    // This is a very hot loop, so it is unrolled
    T* outPtr = static_cast<T*>(outPtr0);
    int m = n * numscalars;
    for (int q = m >> 2; q > 0; --q)
    {
      svtkInterpolateRound(inPtr[0], outPtr[0]);
      svtkInterpolateRound(inPtr[1], outPtr[1]);
      svtkInterpolateRound(inPtr[2], outPtr[2]);
      svtkInterpolateRound(inPtr[3], outPtr[3]);
      inPtr += 4;
      outPtr += 4;
    }
    for (int r = m & 0x0003; r > 0; --r)
    {
      svtkInterpolateRound(*inPtr++, *outPtr++);
    }
    outPtr0 = outPtr;
  }
}

template <class F, class T>
void svtkImageResliceConversion<F, T>::Clamp(void*& outPtr0, const F* inPtr, int numscalars, int n)
{
  T* outPtr = static_cast<T*>(outPtr0);
  for (int m = n * numscalars; m > 0; --m)
  {
    svtkResliceClamp(*inPtr++, *outPtr++);
  }
  outPtr0 = outPtr;
}

// get the conversion function
template <class F>
void svtkGetConversionFunc(void (**conversion)(void*& out, const F* in, int numscalars, int n),
  int inputType, int dataType, double scalarShift, double scalarScale, bool forceClamping)
{
  // make sure that the output values fit in the output data type
  if (dataType != SVTK_FLOAT && dataType != SVTK_DOUBLE && !forceClamping)
  {
    F shift = static_cast<F>(scalarShift);
    F scale = static_cast<F>(scalarScale);
    F checkMin = (static_cast<F>(svtkDataArray::GetDataTypeMin(inputType)) + shift) * scale;
    F checkMax = (static_cast<F>(svtkDataArray::GetDataTypeMax(inputType)) + shift) * scale;
    F outputMin = static_cast<F>(svtkDataArray::GetDataTypeMin(dataType));
    F outputMax = static_cast<F>(svtkDataArray::GetDataTypeMax(dataType));
    if (checkMin > checkMax)
    {
      F tmp = checkMax;
      checkMax = checkMin;
      checkMin = tmp;
    }
    forceClamping = (checkMin < outputMin || checkMax > outputMax);
  }

  if (forceClamping && dataType != SVTK_FLOAT && dataType != SVTK_DOUBLE)
  {
    // clamp to the limits of the output type
    switch (dataType)
    {
      svtkTemplateAliasMacro(*conversion = &(svtkImageResliceConversion<F, SVTK_TT>::Clamp));
      default:
        *conversion = nullptr;
    }
  }
  else
  {
    // clamping is unnecessary, so optimize by skipping the clamp step
    switch (dataType)
    {
      svtkTemplateAliasMacro(*conversion = &(svtkImageResliceConversion<F, SVTK_TT>::Convert));
      default:
        *conversion = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
// Various pixel compositors for slab views
template <class F>
struct svtkImageResliceComposite
{
  static void MeanValue(F* inPtr, int numscalars, int n);
  static void MeanTrap(F* inPtr, int numscalars, int n);
  static void SumValues(F* inPtr, int numscalars, int n);
  static void SumTrap(F* inPtr, int numscalars, int n);
  static void MinValue(F* inPtr, int numscalars, int n);
  static void MaxValue(F* inPtr, int numscalars, int n);
};

template <class F>
void svtkImageResliceSlabSum(F* inPtr, int numscalars, int n, F f)
{
  int m = numscalars;
  --n;
  do
  {
    F result = *inPtr;
    int k = n;
    do
    {
      inPtr += numscalars;
      result += *inPtr;
    } while (--k);
    inPtr -= n * numscalars;
    *inPtr++ = result * f;
  } while (--m);
}

template <class F>
void svtkImageResliceSlabTrap(F* inPtr, int numscalars, int n, F f)
{
  int m = numscalars;
  --n;
  do
  {
    F result = *inPtr * 0.5;
    for (int k = n - 1; k != 0; --k)
    {
      inPtr += numscalars;
      result += *inPtr;
    }
    inPtr += numscalars;
    result += *inPtr * 0.5;
    inPtr -= n * numscalars;
    *inPtr++ = result * f;
  } while (--m);
}

template <class F>
void svtkImageResliceComposite<F>::MeanValue(F* inPtr, int numscalars, int n)
{
  F f = 1.0 / n;
  svtkImageResliceSlabSum(inPtr, numscalars, n, f);
}

template <class F>
void svtkImageResliceComposite<F>::MeanTrap(F* inPtr, int numscalars, int n)
{
  F f = 1.0 / (n - 1);
  svtkImageResliceSlabTrap(inPtr, numscalars, n, f);
}

template <class F>
void svtkImageResliceComposite<F>::SumValues(F* inPtr, int numscalars, int n)
{
  svtkImageResliceSlabSum(inPtr, numscalars, n, static_cast<F>(1.0));
}

template <class F>
void svtkImageResliceComposite<F>::SumTrap(F* inPtr, int numscalars, int n)
{
  svtkImageResliceSlabTrap(inPtr, numscalars, n, static_cast<F>(1.0));
}

template <class F>
void svtkImageResliceComposite<F>::MinValue(F* inPtr, int numscalars, int n)
{
  int m = numscalars;
  --n;
  do
  {
    F result = *inPtr;
    int k = n;
    do
    {
      inPtr += numscalars;
      result = (result < *inPtr ? result : *inPtr);
    } while (--k);
    inPtr -= n * numscalars;
    *inPtr++ = result;
  } while (--m);
}

template <class F>
void svtkImageResliceComposite<F>::MaxValue(F* inPtr, int numscalars, int n)
{
  int m = numscalars;
  --n;
  do
  {
    F result = *inPtr;
    int k = n;
    do
    {
      inPtr += numscalars;
      result = (result > *inPtr ? result : *inPtr);
    } while (--k);
    inPtr -= n * numscalars;
    *inPtr++ = result;
  } while (--m);
}

// get the composite function
template <class F>
void svtkGetCompositeFunc(void (**composite)(F* in, int numscalars, int n), int slabMode, int trpz)
{
  switch (slabMode)
  {
    case SVTK_IMAGE_SLAB_MIN:
      *composite = &(svtkImageResliceComposite<F>::MinValue);
      break;
    case SVTK_IMAGE_SLAB_MAX:
      *composite = &(svtkImageResliceComposite<F>::MaxValue);
      break;
    case SVTK_IMAGE_SLAB_MEAN:
      if (trpz)
      {
        *composite = &(svtkImageResliceComposite<F>::MeanTrap);
      }
      else
      {
        *composite = &(svtkImageResliceComposite<F>::MeanValue);
      }
      break;
    case SVTK_IMAGE_SLAB_SUM:
      if (trpz)
      {
        *composite = &(svtkImageResliceComposite<F>::SumTrap);
      }
      else
      {
        *composite = &(svtkImageResliceComposite<F>::SumValues);
      }
      break;
    default:
      *composite = nullptr;
  }
}

//----------------------------------------------------------------------------
// Some helper functions for 'RequestData'
//----------------------------------------------------------------------------

//--------------------------------------------------------------------------
// pixel copy function, templated for different scalar types
template <class T, int N = 1>
struct svtkImageResliceSetPixels
{
  static void Set(void*& outPtrV, const void* inPtrV, int numscalars, int n)
  {
    const T* inPtr = static_cast<const T*>(inPtrV);
    T* outPtr = static_cast<T*>(outPtrV);
    for (; n > 0; --n)
    {
      const T* tmpPtr = inPtr;
      int m = numscalars;
      do
      {
        *outPtr++ = *tmpPtr++;
      } while (--m);
    }
    outPtrV = outPtr;
  }

  // optimized for 1 scalar components
  static void Set1(void*& outPtrV, const void* inPtrV, int svtkNotUsed(numscalars), int n)
  {
    const T* inPtr = static_cast<const T*>(inPtrV);
    T* outPtr = static_cast<T*>(outPtrV);
    T val = *inPtr;
    for (; n > 0; --n)
    {
      *outPtr++ = val;
    }
    outPtrV = outPtr;
  }

  // optimized for N scalar components
  static void SetN(void*& outPtrV, const void* inPtrV, int svtkNotUsed(numscalars), int n)
  {
    const T* inPtr = static_cast<const T*>(inPtrV);
    T* outPtr = static_cast<T*>(outPtrV);
    for (; n > 0; --n)
    {
      memcpy(outPtr, inPtr, N * sizeof(T));
      outPtr += N;
    }
    outPtrV = outPtr;
  }
};

// get a pixel copy function that is appropriate for the data type
void svtkGetSetPixelsFunc(void (**setpixels)(void*& out, const void* in, int numscalars, int n),
  int dataType, int numscalars)
{
  switch (numscalars)
  {
    case 1:
      switch (dataType)
      {
        svtkTemplateAliasMacro(*setpixels = &svtkImageResliceSetPixels<SVTK_TT>::Set1);
        default:
          *setpixels = nullptr;
      }
      break;
    case 2:
      switch (dataType)
      {
        svtkTemplateAliasMacro(*setpixels = &(svtkImageResliceSetPixels<SVTK_TT, 2>::SetN));
        default:
          *setpixels = nullptr;
      }
      break;
    case 3:
      switch (dataType)
      {
        svtkTemplateAliasMacro(*setpixels = &(svtkImageResliceSetPixels<SVTK_TT, 3>::SetN));
        default:
          *setpixels = nullptr;
      }
      break;
    case 4:
      switch (dataType)
      {
        svtkTemplateAliasMacro(*setpixels = &(svtkImageResliceSetPixels<SVTK_TT, 4>::SetN));
        default:
          *setpixels = nullptr;
      }
      break;
    default:
      switch (dataType)
      {
        svtkTemplateAliasMacro(*setpixels = &svtkImageResliceSetPixels<SVTK_TT>::Set);
        default:
          *setpixels = nullptr;
      }
  }
}

//----------------------------------------------------------------------------
// Convert background color from double to appropriate type
template <class T>
void svtkCopyBackgroundColor(double dcolor[4], T* background, int numComponents)
{
  int c = (numComponents < 4 ? numComponents : 4);
  for (int i = 0; i < c; i++)
  {
    svtkResliceClamp(dcolor[i], background[i]);
  }
  for (int j = c; j < numComponents; j++)
  {
    background[j] = 0;
  }
}

void svtkAllocBackgroundPixel(
  void** rval, double dcolor[4], int scalarType, int scalarSize, int numComponents)
{
  int bytesPerPixel = numComponents * scalarSize;

  // allocate as an array of doubles to guarantee alignment
  // (this is probably more paranoid than necessary)
  int n = (bytesPerPixel + SVTK_SIZEOF_DOUBLE - 1) / SVTK_SIZEOF_DOUBLE;
  double* doublePtr = new double[n];
  *rval = doublePtr;

  switch (scalarType)
  {
    svtkTemplateAliasMacro(svtkCopyBackgroundColor(dcolor, (SVTK_TT*)(*rval), numComponents));
  }
}

void svtkFreeBackgroundPixel(void** rval)
{
  double* doublePtr = static_cast<double*>(*rval);
  delete[] doublePtr;

  *rval = nullptr;
}

//----------------------------------------------------------------------------
// helper function for rescaling the data
template <class F>
void svtkImageResliceRescaleScalars(
  F* floatData, int components, int n, double scalarShift, double scalarScale)
{
  svtkIdType m = n;
  m *= components;
  F shift = static_cast<F>(scalarShift);
  F scale = static_cast<F>(scalarScale);
  for (svtkIdType i = 0; i < m; i++)
  {
    *floatData = (*floatData + shift) * scale;
    floatData++;
  }
}

//----------------------------------------------------------------------------
// This function simply clears the entire output to the background color,
// for cases where the transformation places the output extent completely
// outside of the input extent.
void svtkImageResliceClearExecute(
  svtkImageReslice* self, svtkImageData* outData, void* outPtr, int outExt[6], int threadId)
{
  void (*setpixels)(void*& out, const void* in, int numscalars, int n) = nullptr;

  // Get Increments to march through data
  svtkIdType outIncX, outIncY, outIncZ;
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  int scalarType = outData->GetScalarType();
  int scalarSize = outData->GetScalarSize();
  int numscalars = outData->GetNumberOfScalarComponents();

  // allocate a voxel to copy into the background (out-of-bounds) regions
  void* background;
  svtkAllocBackgroundPixel(
    &background, self->GetBackgroundColor(), scalarType, scalarSize, numscalars);
  // get the appropriate function for pixel copying
  svtkGetSetPixelsFunc(&setpixels, scalarType, numscalars);

  svtkImagePointDataIterator iter(outData, outExt, nullptr, self, threadId);
  for (; !iter.IsAtEnd(); iter.NextSpan())
  {
    // clear the pixels to background color and go to next row
    outPtr = iter.GetVoidPointer(outData, iter.GetId());
    setpixels(outPtr, background, numscalars, outExt[1] - outExt[0] + 1);
  }

  svtkFreeBackgroundPixel(&background);
}

//----------------------------------------------------------------------------
// application of the transform has different forms for fixed-point
// vs. floating-point
template <class F>
void svtkResliceApplyTransform(
  svtkAbstractTransform* newtrans, F inPoint[3], F inOrigin[3], F inInvSpacing[3])
{
  newtrans->InternalTransformPoint(inPoint, inPoint);
  inPoint[0] -= inOrigin[0];
  inPoint[1] -= inOrigin[1];
  inPoint[2] -= inOrigin[2];
  inPoint[0] *= inInvSpacing[0];
  inPoint[1] *= inInvSpacing[1];
  inPoint[2] *= inInvSpacing[2];
}

//----------------------------------------------------------------------------
// the main execute function
template <class F>
void svtkImageResliceExecute(svtkImageReslice* self, svtkDataArray* scalars,
  svtkAbstractImageInterpolator* interpolator, svtkImageData* outData, void* outPtr,
  double scalarShift, double scalarScale, svtkImageResliceConvertScalarsType convertScalars,
  int outExt[6], int threadId, F newmat[4][4], svtkAbstractTransform* newtrans)
{
  void (*convertpixels)(void*& out, const F* in, int numscalars, int n) = nullptr;
  void (*setpixels)(void*& out, const void* in, int numscalars, int n) = nullptr;
  void (*composite)(F * in, int numscalars, int n) = nullptr;

  // get the input stencil
  svtkImageStencilData* stencil = self->GetStencil();
  // get the output stencil
  svtkImageStencilData* outputStencil = nullptr;
  if (self->GetGenerateStencilOutput())
  {
    outputStencil = self->GetStencilOutput();
  }

  // multiple samples for thick slabs
  int nsamples = self->GetSlabNumberOfSlices();
  nsamples = ((nsamples > 1) ? nsamples : 1);

  // spacing between slab samples (as a fraction of slice spacing).
  double slabSampleSpacing = self->GetSlabSliceSpacingFraction();

  // check for perspective transformation
  bool perspective = 0;
  if (newmat[3][0] != 0 || newmat[3][1] != 0 || newmat[3][2] != 0 || newmat[3][3] != 1)
  {
    perspective = 1;
  }

  // extra scalar info for nearest-neighbor optimization
  const void* inPtr = scalars->GetVoidPointer(0);
  int inputScalarSize = scalars->GetDataTypeSize();
  int inputScalarType = scalars->GetDataType();
  int inComponents = interpolator->GetNumberOfComponents();
  int componentOffset = interpolator->GetComponentOffset();
  int borderMode = interpolator->GetBorderMode();
  const int* inExt = interpolator->GetExtent();
  svtkIdType inInc[3];
  inInc[0] = scalars->GetNumberOfComponents();
  inInc[1] = inInc[0] * (inExt[1] - inExt[0] + 1);
  inInc[2] = inInc[1] * (inExt[3] - inExt[2] + 1);
  svtkIdType fullSize = (inExt[1] - inExt[0] + 1);
  fullSize *= (inExt[3] - inExt[2] + 1);
  fullSize *= (inExt[5] - inExt[4] + 1);
  if (componentOffset > 0 && componentOffset + inComponents < inInc[0])
  {
    inPtr = static_cast<const char*>(inPtr) + inputScalarSize * componentOffset;
  }

  int interpolationMode = SVTK_INT_MAX;
  if (interpolator->IsA("svtkImageInterpolator"))
  {
    interpolationMode = static_cast<svtkImageInterpolator*>(interpolator)->GetInterpolationMode();
  }

  bool rescaleScalars = (scalarShift != 0.0 || scalarScale != 1.0);

  // is nearest neighbor optimization possible?
  bool optimizeNearest = 0;
  if (interpolationMode == SVTK_NEAREST_INTERPOLATION && borderMode == SVTK_IMAGE_BORDER_CLAMP &&
    !(newtrans || perspective || convertScalars || rescaleScalars) &&
    inputScalarType == outData->GetScalarType() && fullSize == scalars->GetNumberOfTuples() &&
    self->GetBorder() == 1 && nsamples <= 1)
  {
    optimizeNearest = 1;
  }

  // get pixel information
  int scalarType = outData->GetScalarType();
  int scalarSize = outData->GetScalarSize();
  int outComponents = outData->GetNumberOfScalarComponents();

  // break matrix into a set of axes plus an origin
  // (this allows us to calculate the transform Incrementally)
  F xAxis[4], yAxis[4], zAxis[4], origin[4];
  for (int i = 0; i < 4; i++)
  {
    xAxis[i] = newmat[i][0];
    yAxis[i] = newmat[i][1];
    zAxis[i] = newmat[i][2];
    origin[i] = newmat[i][3];
  }

  // get the input origin and spacing for conversion purposes
  double temp[3];
  F inOrigin[3];
  interpolator->GetOrigin(temp);
  inOrigin[0] = F(temp[0]);
  inOrigin[1] = F(temp[1]);
  inOrigin[2] = F(temp[2]);

  F inInvSpacing[3];
  interpolator->GetSpacing(temp);
  inInvSpacing[0] = F(1.0 / temp[0]);
  inInvSpacing[1] = F(1.0 / temp[1]);
  inInvSpacing[2] = F(1.0 / temp[2]);

  // allocate an output row of type double
  F* floatPtr = nullptr;
  if (!optimizeNearest)
  {
    floatPtr = new F[inComponents * (outExt[1] - outExt[0] + nsamples)];
  }

  // set color for area outside of input volume extent
  void* background;
  svtkAllocBackgroundPixel(
    &background, self->GetBackgroundColor(), scalarType, scalarSize, outComponents);

  // get various helper functions
  bool forceClamping = (interpolationMode > SVTK_RESLICE_LINEAR ||
    (nsamples > 1 && self->GetSlabMode() == SVTK_IMAGE_SLAB_SUM));
  svtkGetConversionFunc(
    &convertpixels, inputScalarType, scalarType, scalarShift, scalarScale, forceClamping);
  svtkGetSetPixelsFunc(&setpixels, scalarType, outComponents);
  svtkGetCompositeFunc(&composite, self->GetSlabMode(), self->GetSlabTrapezoidIntegration());

  // create some variables for when we march through the data
  int idY = outExt[2] - 1;
  int idZ = outExt[4] - 1;
  F inPoint0[4] = { 0.0, 0.0, 0.0, 0.0 };
  F inPoint1[4] = { 0.0, 0.0, 0.0, 0.0 };

  // create an iterator to march through the data
  svtkImagePointDataIterator iter(outData, outExt, stencil, self, threadId);
  char* outPtr0 = static_cast<char*>(iter.GetVoidPointer(outData));
  for (; !iter.IsAtEnd(); iter.NextSpan())
  {
    int span = static_cast<int>(iter.SpanEndId() - iter.GetId());
    outPtr = outPtr0 + iter.GetId() * scalarSize * outComponents;

    if (!iter.IsInStencil())
    {
      // clear any regions that are outside the stencil
      setpixels(outPtr, background, outComponents, span);
    }
    else
    {
      // get output index, and compute position in input image
      int outIndex[3];
      iter.GetIndex(outIndex);

      // if Z index increased, then advance position along Z axis
      if (outIndex[2] > idZ)
      {
        idZ = outIndex[2];
        inPoint0[0] = origin[0] + idZ * zAxis[0];
        inPoint0[1] = origin[1] + idZ * zAxis[1];
        inPoint0[2] = origin[2] + idZ * zAxis[2];
        inPoint0[3] = origin[3] + idZ * zAxis[3];
        idY = outExt[2] - 1;
      }

      // if Y index increased, then advance position along Y axis
      if (outIndex[1] > idY)
      {
        idY = outIndex[1];
        inPoint1[0] = inPoint0[0] + idY * yAxis[0];
        inPoint1[1] = inPoint0[1] + idY * yAxis[1];
        inPoint1[2] = inPoint0[2] + idY * yAxis[2];
        inPoint1[3] = inPoint0[3] + idY * yAxis[3];
      }

      // march through one row of the output image
      int idXmin = outIndex[0];
      int idXmax = idXmin + span - 1;

      if (!optimizeNearest)
      {
        bool wasInBounds = 1;
        bool isInBounds = 1;
        int startIdX = idXmin;
        int idX = idXmin;
        F* tmpPtr = floatPtr;

        while (startIdX <= idXmax)
        {
          for (; idX <= idXmax && isInBounds == wasInBounds; idX++)
          {
            F inPoint2[4];
            inPoint2[0] = inPoint1[0] + idX * xAxis[0];
            inPoint2[1] = inPoint1[1] + idX * xAxis[1];
            inPoint2[2] = inPoint1[2] + idX * xAxis[2];
            inPoint2[3] = inPoint1[3] + idX * xAxis[3];

            F inPoint3[4];
            F* inPoint = inPoint2;
            isInBounds = 0;

            int sampleCount = 0;
            for (int sample = 0; sample < nsamples; sample++)
            {
              if (nsamples > 1)
              {
                double s = sample - 0.5 * (nsamples - 1);
                s *= slabSampleSpacing;
                inPoint3[0] = inPoint2[0] + s * zAxis[0];
                inPoint3[1] = inPoint2[1] + s * zAxis[1];
                inPoint3[2] = inPoint2[2] + s * zAxis[2];
                inPoint3[3] = inPoint2[3] + s * zAxis[3];
                inPoint = inPoint3;
              }

              if (perspective)
              { // only do perspective if necessary
                F f = 1 / inPoint[3];
                inPoint[0] *= f;
                inPoint[1] *= f;
                inPoint[2] *= f;
              }

              if (newtrans)
              { // apply the AbstractTransform if there is one
                svtkResliceApplyTransform(newtrans, inPoint, inOrigin, inInvSpacing);
              }

              if (interpolator->CheckBoundsIJK(inPoint))
              {
                // do the interpolation
                sampleCount++;
                isInBounds = 1;
                interpolator->InterpolateIJK(inPoint, tmpPtr);
                tmpPtr += inComponents;
              }
            }

            tmpPtr -= sampleCount * inComponents;
            if (sampleCount > 1)
            {
              composite(tmpPtr, inComponents, sampleCount);
            }
            tmpPtr += inComponents;

            // set "was in" to "is in" if first pixel
            wasInBounds = ((idX > idXmin) ? wasInBounds : isInBounds);
          }

          // write a segment to the output
          int endIdX = idX - 1 - (isInBounds != wasInBounds);
          int numpixels = endIdX - startIdX + 1;

          if (wasInBounds)
          {
            if (outputStencil)
            {
              outputStencil->InsertNextExtent(startIdX, endIdX, idY, idZ);
            }

            if (rescaleScalars)
            {
              svtkImageResliceRescaleScalars(
                floatPtr, inComponents, idXmax - idXmin + 1, scalarShift, scalarScale);
            }

            if (convertScalars)
            {
              (self->*convertScalars)(tmpPtr - inComponents * (idX - startIdX), outPtr,
                svtkTypeTraits<F>::SVTKTypeID(), inComponents, numpixels, startIdX, idY, idZ,
                threadId);

              outPtr = static_cast<char*>(outPtr) + numpixels * outComponents * scalarSize;
            }
            else
            {
              convertpixels(
                outPtr, tmpPtr - inComponents * (idX - startIdX), outComponents, numpixels);
            }
          }
          else
          {
            setpixels(outPtr, background, outComponents, numpixels);
          }

          startIdX += numpixels;
          wasInBounds = isInBounds;
        }
      }
      else // optimize for nearest-neighbor interpolation
      {
        const char* inPtrTmp0 = static_cast<const char*>(inPtr);
        char* outPtrTmp = static_cast<char*>(outPtr);

        svtkIdType inIncX = inInc[0] * inputScalarSize;
        svtkIdType inIncY = inInc[1] * inputScalarSize;
        svtkIdType inIncZ = inInc[2] * inputScalarSize;

        int inExtX = inExt[1] - inExt[0] + 1;
        int inExtY = inExt[3] - inExt[2] + 1;
        int inExtZ = inExt[5] - inExt[4] + 1;

        int startIdX = idXmin;
        int endIdX = idXmin - 1;
        bool isInBounds = false;
        int bytesPerPixel = inputScalarSize * inComponents;

        for (int iidX = idXmin; iidX <= idXmax; iidX++)
        {
          F inPoint[3];
          inPoint[0] = inPoint1[0] + iidX * xAxis[0];
          inPoint[1] = inPoint1[1] + iidX * xAxis[1];
          inPoint[2] = inPoint1[2] + iidX * xAxis[2];

          int inIdX = svtkInterpolationMath::Round(inPoint[0]) - inExt[0];
          int inIdY = svtkInterpolationMath::Round(inPoint[1]) - inExt[2];
          int inIdZ = svtkInterpolationMath::Round(inPoint[2]) - inExt[4];

          if (inIdX >= 0 && inIdX < inExtX && inIdY >= 0 && inIdY < inExtY && inIdZ >= 0 &&
            inIdZ < inExtZ)
          {
            if (!isInBounds)
            {
              // clear leading out-of-bounds pixels
              startIdX = iidX;
              isInBounds = true;
              setpixels(outPtr, background, outComponents, startIdX - idXmin);
              outPtrTmp = static_cast<char*>(outPtr);
            }
            // set the final index that was within input bounds
            endIdX = iidX;

            // perform nearest-neighbor interpolation via pixel copy
            const char* inPtrTmp = inPtrTmp0 + inIdX * inIncX + inIdY * inIncY + inIdZ * inIncZ;

            // when memcpy is used with a constant size, the compiler will
            // optimize away the function call and use the minimum number
            // of instructions necessary to perform the copy
            switch (bytesPerPixel)
            {
              case 1:
                outPtrTmp[0] = inPtrTmp[0];
                break;
              case 2:
                memcpy(outPtrTmp, inPtrTmp, 2);
                break;
              case 3:
                memcpy(outPtrTmp, inPtrTmp, 3);
                break;
              case 4:
                memcpy(outPtrTmp, inPtrTmp, 4);
                break;
              case 8:
                memcpy(outPtrTmp, inPtrTmp, 8);
                break;
              case 12:
                memcpy(outPtrTmp, inPtrTmp, 12);
                break;
              case 16:
                memcpy(outPtrTmp, inPtrTmp, 16);
                break;
              default:
                int oc = 0;
                do
                {
                  outPtrTmp[oc] = inPtrTmp[oc];
                } while (++oc != bytesPerPixel);
                break;
            }
            outPtrTmp += bytesPerPixel;
          }
          else if (isInBounds)
          {
            // leaving input bounds
            break;
          }
        }

        // clear trailing out-of-bounds pixels
        outPtr = outPtrTmp;
        setpixels(outPtr, background, outComponents, idXmax - endIdX);

        if (outputStencil && endIdX >= startIdX)
        {
          outputStencil->InsertNextExtent(startIdX, endIdX, idY, idZ);
        }
      }
    }
  }

  svtkFreeBackgroundPixel(&background);

  if (!optimizeNearest)
  {
    delete[] floatPtr;
  }
}

//----------------------------------------------------------------------------
// svtkReslicePermuteExecute is specifically optimized for
// cases where the IndexMatrix has only one non-zero component
// per row, i.e. when the matrix is permutation+scale+translation.
// All of the interpolation coefficients are calculated ahead
// of time instead of on a pixel-by-pixel basis.

namespace
{

//----------------------------------------------------------------------------
// Optimized routines for nearest-neighbor interpolation

template <class T, int N = 1>
struct svtkImageResliceRowInterpolate
{
  static void Nearest(
    void*& outPtr0, int idX, int idY, int idZ, int, int n, const svtkInterpolationWeights* weights);

  static void Nearest1(
    void*& outPtr0, int idX, int idY, int idZ, int, int n, const svtkInterpolationWeights* weights);

  static void NearestN(
    void*& outPtr0, int idX, int idY, int idZ, int, int n, const svtkInterpolationWeights* weights);
};

//----------------------------------------------------------------------------
// helper function for nearest neighbor interpolation
template <class T, int N>
void svtkImageResliceRowInterpolate<T, N>::Nearest(void*& outPtr0, int idX, int idY, int idZ,
  int numscalars, int n, const svtkInterpolationWeights* weights)
{
  const svtkIdType* iX = weights->Positions[0] + idX;
  const svtkIdType* iY = weights->Positions[1] + idY;
  const svtkIdType* iZ = weights->Positions[2] + idZ;
  const T* inPtr0 = static_cast<const T*>(weights->Pointer) + iY[0] + iZ[0];
  T* outPtr = static_cast<T*>(outPtr0);

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = n; i > 0; --i)
  {
    const T* tmpPtr = &inPtr0[iX[0]];
    iX++;
    int m = numscalars;
    do
    {
      *outPtr++ = *tmpPtr++;
    } while (--m);
  }
  outPtr0 = outPtr;
}

//----------------------------------------------------------------------------
// specifically for 1 scalar component
template <class T, int N>
void svtkImageResliceRowInterpolate<T, N>::Nearest1(
  void*& outPtr0, int idX, int idY, int idZ, int, int n, const svtkInterpolationWeights* weights)
{
  const svtkIdType* iX = weights->Positions[0] + idX;
  const svtkIdType* iY = weights->Positions[1] + idY;
  const svtkIdType* iZ = weights->Positions[2] + idZ;
  const T* inPtr0 = static_cast<const T*>(weights->Pointer) + iY[0] + iZ[0];
  T* outPtr = static_cast<T*>(outPtr0);

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = n; i > 0; --i)
  {
    const T* tmpPtr = &inPtr0[iX[0]];
    iX++;
    *outPtr++ = *tmpPtr;
  }
  outPtr0 = outPtr;
}

//----------------------------------------------------------------------------
// specifically for N scalar components
template <class T, int N>
void svtkImageResliceRowInterpolate<T, N>::NearestN(
  void*& outPtr0, int idX, int idY, int idZ, int, int n, const svtkInterpolationWeights* weights)
{
  const svtkIdType* iX = weights->Positions[0] + idX;
  const svtkIdType* iY = weights->Positions[1] + idY;
  const svtkIdType* iZ = weights->Positions[2] + idZ;
  const T* inPtr0 = static_cast<const T*>(weights->Pointer) + iY[0] + iZ[0];
  T* outPtr = static_cast<T*>(outPtr0);

  // This is a hot loop.
  // Be very careful changing it, as it affects performance greatly.
  for (int i = n; i > 0; --i)
  {
    const T* tmpPtr = &inPtr0[iX[0]];
    iX++;
    memcpy(outPtr, tmpPtr, N * sizeof(T));
    outPtr += N;
  }
  outPtr0 = outPtr;
}

//----------------------------------------------------------------------------
// get row interpolation function for different interpolation modes
// and different scalar types
void svtkGetSummationFunc(void (**summation)(void*& outPtr, int idX, int idY, int idZ,
                           int numscalars, int n, const svtkInterpolationWeights* weights),
  int scalarType, int numScalars)
{
  *summation = nullptr;

  if (numScalars == 1)
  {
    switch (scalarType)
    {
      svtkTemplateAliasMacro(*summation = &(svtkImageResliceRowInterpolate<SVTK_TT>::Nearest1));
      default:
        *summation = nullptr;
    }
  }
  else if (numScalars == 2)
  {
    switch (scalarType)
    {
      svtkTemplateAliasMacro(*summation = &(svtkImageResliceRowInterpolate<SVTK_TT, 2>::NearestN));
      default:
        *summation = nullptr;
    }
  }
  else if (numScalars == 3)
  {
    switch (scalarType)
    {
      svtkTemplateAliasMacro(*summation = &(svtkImageResliceRowInterpolate<SVTK_TT, 3>::NearestN));
      default:
        *summation = nullptr;
    }
  }
  else if (numScalars == 4)
  {
    switch (scalarType)
    {
      svtkTemplateAliasMacro(*summation = &(svtkImageResliceRowInterpolate<SVTK_TT, 4>::NearestN));
      default:
        *summation = nullptr;
    }
  }
  else
  {
    switch (scalarType)
    {
      svtkTemplateAliasMacro(*summation = &(svtkImageResliceRowInterpolate<SVTK_TT>::Nearest));
      default:
        *summation = nullptr;
    }
  }
}

//----------------------------------------------------------------------------
template <class F>
struct svtkImageResliceRowComp
{
  static void SumRow(F* op, const F* ip, int nc, int m, int i, int n);
  static void SumRowTrap(F* op, const F* ip, int nc, int m, int i, int n);
  static void MeanRow(F* op, const F* ip, int nc, int m, int i, int n);
  static void MeanRowTrap(F* op, const F* ip, int nc, int m, int i, int n);
  static void MinRow(F* op, const F* ip, int nc, int m, int i, int n);
  static void MaxRow(F* op, const F* ip, int nc, int m, int i, int n);
};

template <class F>
void svtkImageResliceRowComp<F>::SumRow(
  F* outPtr, const F* inPtr, int numComp, int count, int i, int)
{
  int m = count * numComp;
  if (m)
  {
    if (i == 0)
    {
      do
      {
        *outPtr++ = *inPtr++;
      } while (--m);
    }
    else
    {
      do
      {
        *outPtr++ += *inPtr++;
      } while (--m);
    }
  }
}

template <class F>
void svtkImageResliceRowComp<F>::SumRowTrap(
  F* outPtr, const F* inPtr, int numComp, int count, int i, int n)
{
  int m = count * numComp;
  if (m)
  {
    if (i == 0)
    {
      do
      {
        *outPtr++ = 0.5 * (*inPtr++);
      } while (--m);
    }
    else if (i == n - 1)
    {
      do
      {
        *outPtr++ += 0.5 * (*inPtr++);
      } while (--m);
    }
    else
    {
      do
      {
        *outPtr++ += *inPtr++;
      } while (--m);
    }
  }
}

template <class F>
void svtkImageResliceRowComp<F>::MeanRow(
  F* outPtr, const F* inPtr, int numComp, int count, int i, int n)
{
  int m = count * numComp;
  if (m)
  {
    if (i == 0)
    {
      do
      {
        *outPtr++ = *inPtr++;
      } while (--m);
    }
    else if (i == n - 1)
    {
      F f = F(1.0 / n);
      do
      {
        *outPtr += *inPtr++;
        *outPtr *= f;
        outPtr++;
      } while (--m);
    }
    else
    {
      do
      {
        *outPtr++ += *inPtr++;
      } while (--m);
    }
  }
}

template <class F>
void svtkImageResliceRowComp<F>::MeanRowTrap(
  F* outPtr, const F* inPtr, int numComp, int count, int i, int n)
{
  int m = count * numComp;
  if (m)
  {
    if (i == 0)
    {
      do
      {
        *outPtr++ = 0.5 * (*inPtr++);
      } while (--m);
    }
    else if (i == n - 1)
    {
      F f = F(1.0 / (n - 1));
      do
      {
        *outPtr += 0.5 * (*inPtr++);
        *outPtr *= f;
        outPtr++;
      } while (--m);
    }
    else
    {
      do
      {
        *outPtr++ += *inPtr++;
      } while (--m);
    }
  }
}

template <class F>
void svtkImageResliceRowComp<F>::MinRow(
  F* outPtr, const F* inPtr, int numComp, int count, int i, int)
{
  int m = count * numComp;
  if (m)
  {
    if (i == 0)
    {
      do
      {
        *outPtr++ = *inPtr++;
      } while (--m);
    }
    else
    {
      do
      {
        *outPtr = ((*outPtr < *inPtr) ? *outPtr : *inPtr);
        outPtr++;
        inPtr++;
      } while (--m);
    }
  }
}

template <class F>
void svtkImageResliceRowComp<F>::MaxRow(
  F* outPtr, const F* inPtr, int numComp, int count, int i, int)
{
  int m = count * numComp;
  if (m)
  {
    if (i == 0)
    {
      do
      {
        *outPtr++ = *inPtr++;
      } while (--m);
    }
    else
    {
      do
      {
        *outPtr = ((*outPtr > *inPtr) ? *outPtr : *inPtr);
        outPtr++;
        inPtr++;
      } while (--m);
    }
  }
}

// get the composite function
template <class F>
void svtkGetRowCompositeFunc(
  void (**composite)(F* op, const F* ip, int nc, int count, int i, int n), int slabMode, int trpz)
{
  switch (slabMode)
  {
    case SVTK_IMAGE_SLAB_MIN:
      *composite = &(svtkImageResliceRowComp<F>::MinRow);
      break;
    case SVTK_IMAGE_SLAB_MAX:
      *composite = &(svtkImageResliceRowComp<F>::MaxRow);
      break;
    case SVTK_IMAGE_SLAB_MEAN:
      if (trpz)
      {
        *composite = &(svtkImageResliceRowComp<F>::MeanRowTrap);
      }
      else
      {
        *composite = &(svtkImageResliceRowComp<F>::MeanRow);
      }
      break;
    case SVTK_IMAGE_SLAB_SUM:
      if (trpz)
      {
        *composite = &(svtkImageResliceRowComp<F>::SumRowTrap);
      }
      else
      {
        *composite = &(svtkImageResliceRowComp<F>::SumRow);
      }
      break;
    default:
      svtkGenericWarningMacro("Illegal slab mode!");
      *composite = nullptr;
  }
}

} // end anonymous namespace

//----------------------------------------------------------------------------
// the ReslicePermuteExecute path is taken when the output slices are
// orthogonal to the input slices
template <class F>
void svtkReslicePermuteExecute(svtkImageReslice* self, svtkDataArray* scalars,
  svtkAbstractImageInterpolator* interpolator, svtkImageData* outData, void* outPtr,
  double scalarShift, double scalarScale, svtkImageResliceConvertScalarsType convertScalars,
  int outExt[6], int threadId, F matrix[4][4])
{
  // Get Increments to march through data
  svtkIdType outIncX, outIncY, outIncZ;
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  int scalarType = outData->GetScalarType();
  int scalarSize = outData->GetScalarSize();
  int outComponents = outData->GetNumberOfScalarComponents();

  // slab mode
  int nsamples = self->GetSlabNumberOfSlices();
  nsamples = ((nsamples > 0) ? nsamples : 1);
  F(*newmat)[4];
  newmat = matrix;
  F smatrix[4][4];
  int* extent = outExt;
  int sextent[6];
  if (nsamples > 1)
  {
    F* tmpm1 = *matrix;
    F* tmpm2 = *smatrix;
    for (int ii = 0; ii < 16; ii++)
    {
      *tmpm2++ = *tmpm1++;
    }
    smatrix[0][3] -= 0.5 * smatrix[0][2] * nsamples;
    smatrix[1][3] -= 0.5 * smatrix[1][2] * nsamples;
    smatrix[2][3] -= 0.5 * smatrix[2][2] * nsamples;
    newmat = smatrix;
    for (int jj = 0; jj < 6; jj++)
    {
      sextent[jj] = outExt[jj];
    }
    sextent[5] += nsamples - 1;
    extent = sextent;
  }

  // get the input stencil
  svtkImageStencilData* stencil = self->GetStencil();
  // get the output stencil
  svtkImageStencilData* outputStencil = nullptr;
  if (self->GetGenerateStencilOutput())
  {
    outputStencil = self->GetStencilOutput();
  }

  bool rescaleScalars = (scalarShift != 0.0 || scalarScale != 1.0);

  // get the interpolation mode from the interpolator
  int interpolationMode = SVTK_INT_MAX;
  if (interpolator->IsA("svtkImageInterpolator"))
  {
    interpolationMode = static_cast<svtkImageInterpolator*>(interpolator)->GetInterpolationMode();
  }

  // if doConversion is false, a special fast-path will be used
  bool doConversion = true;
  int inputScalarType = scalars->GetDataType();
  if (interpolationMode == SVTK_NEAREST_INTERPOLATION && inputScalarType == scalarType &&
    !convertScalars && !rescaleScalars && nsamples == 1)
  {
    doConversion = false;
  }

  // useful information from the interpolator
  int inComponents = interpolator->GetNumberOfComponents();

  // fill in the interpolation tables
  int clipExt[6];
  svtkInterpolationWeights* weights;
  interpolator->PrecomputeWeightsForExtent(*newmat, extent, clipExt, weights);

  // get type-specific functions
  void (*summation)(void*& out, int idX, int idY, int idZ, int numscalars, int n,
    const svtkInterpolationWeights* weights) = nullptr;
  void (*conversion)(void*& out, const F* in, int numscalars, int n) = nullptr;
  void (*setpixels)(void*& out, const void* in, int numscalars, int n) = nullptr;
  svtkGetSummationFunc(&summation, scalarType, outComponents);
  bool forceClamping = (interpolationMode > SVTK_RESLICE_LINEAR ||
    (nsamples > 1 && self->GetSlabMode() == SVTK_IMAGE_SLAB_SUM));
  svtkGetConversionFunc(
    &conversion, inputScalarType, scalarType, scalarShift, scalarScale, forceClamping);
  svtkGetSetPixelsFunc(&setpixels, scalarType, outComponents);

  // get the slab compositing function
  void (*composite)(F * op, const F* ip, int nc, int count, int i, int n) = nullptr;
  svtkGetRowCompositeFunc(&composite, self->GetSlabMode(), self->GetSlabTrapezoidIntegration());

  // get temp float space for type conversion
  F* floatPtr = nullptr;
  F* floatSumPtr = nullptr;
  if (doConversion)
  {
    floatPtr = new F[inComponents * (outExt[1] - outExt[0] + 1)];
  }
  if (nsamples > 1)
  {
    floatSumPtr = new F[inComponents * (outExt[1] - outExt[0] + 1)];
  }

  // set color for area outside of input volume extent
  void* background;
  svtkAllocBackgroundPixel(
    &background, self->GetBackgroundColor(), scalarType, scalarSize, outComponents);

  // generate the extent we will iterate over while painting output
  // voxels with input data (anything outside will be background color)
  int iterExt[6];
  bool empty = false;
  for (int jj = 0; jj < 6; jj += 2)
  {
    iterExt[jj] = clipExt[jj];
    iterExt[jj + 1] = clipExt[jj + 1];
    empty |= (iterExt[jj] > iterExt[jj + 1]);
  }
  if (empty)
  {
    for (int jj = 0; jj < 6; jj += 2)
    {
      iterExt[jj] = outExt[jj];
      iterExt[jj + 1] = outExt[jj] - 1;
    }
  }
  else if (nsamples > 1)
  {
    // adjust extent for multiple samples if slab mode
    int adjust = nsamples - 1;
    int maxAdjustDown = iterExt[4] - outExt[4];
    iterExt[4] -= (adjust <= maxAdjustDown ? adjust : maxAdjustDown);
    int maxAdjustUp = outExt[5] - iterExt[5];
    iterExt[5] += (adjust <= maxAdjustUp ? adjust : maxAdjustUp);
  }

  // clear any leading slices
  for (int idZ = outExt[4]; idZ < iterExt[4]; idZ++)
  {
    int fullspan = outExt[1] - outExt[0] + 1;
    for (int idY = outExt[2]; idY <= outExt[3]; idY++)
    {
      setpixels(outPtr, background, outComponents, fullspan);
      outPtr = static_cast<char*>(outPtr) + outIncY * scalarSize;
    }
    outPtr = static_cast<char*>(outPtr) + outIncZ * scalarSize;
  }

  if (!empty)
  {
    svtkImagePointDataIterator iter(outData, iterExt, stencil, self, threadId);
    for (; !iter.IsAtEnd(); iter.NextSpan())
    {
      // get output index
      int outIndex[3];
      iter.GetIndex(outIndex);
      int span = static_cast<int>(iter.SpanEndId() - iter.GetId());
      int idXmin = outIndex[0];
      int idXmax = idXmin + span - 1;
      int idY = outIndex[1];
      int idZ = outIndex[2];

      if (idXmin == iterExt[0])
      {
        // clear rows that were outside of iterExt
        if (idY == iterExt[2])
        {
          int fullspan = outExt[1] - outExt[0] + 1;
          for (idY = outExt[2]; idY < iterExt[2]; idY++)
          {
            setpixels(outPtr, background, outComponents, fullspan);
            outPtr = static_cast<char*>(outPtr) + outIncY * scalarSize;
          }
        }
        // clear leading pixels
        if (iterExt[0] > outExt[0])
        {
          setpixels(outPtr, background, outComponents, iterExt[0] - outExt[0]);
        }
      }

      if (!iter.IsInStencil())
      {
        // clear any regions that are outside the stencil
        setpixels(outPtr, background, outComponents, span);
      }
      else
      {
        int idX = idXmin;

        if (doConversion)
        {
          // these six lines are for handling incomplete slabs
          int lowerSkip = clipExt[4] - idZ;
          lowerSkip = (lowerSkip >= 0 ? lowerSkip : 0);
          int upperSkip = idZ + (nsamples - 1) - clipExt[5];
          upperSkip = (upperSkip >= 0 ? upperSkip : 0);
          int idZ1 = idZ + lowerSkip;
          int nsamples1 = nsamples - lowerSkip - upperSkip;

          for (int isample = 0; isample < nsamples1; isample++)
          {
            F* tmpPtr = ((nsamples1 > 1) ? floatSumPtr : floatPtr);
            interpolator->InterpolateRow(weights, idX, idY, idZ1, tmpPtr, span);

            if (composite && (nsamples1 > 1))
            {
              composite(floatPtr, floatSumPtr, inComponents, span, isample, nsamples1);
            }

            idZ1++;
          }

          if (rescaleScalars)
          {
            svtkImageResliceRescaleScalars(floatPtr, inComponents, span, scalarShift, scalarScale);
          }

          if (convertScalars)
          {
            (self->*convertScalars)(floatPtr, outPtr, svtkTypeTraits<F>::SVTKTypeID(), inComponents,
              span, idXmin, idY, idZ, threadId);

            outPtr =
              (static_cast<char*>(outPtr) + static_cast<size_t>(span) * outComponents * scalarSize);
          }
          else
          {
            conversion(outPtr, floatPtr, inComponents, span);
          }
        }
        else
        {
          // fast path for when no conversion is necessary
          summation(outPtr, idX, idY, idZ, inComponents, span, weights);
        }

        if (outputStencil)
        {
          outputStencil->InsertNextExtent(idXmin, idXmax, idY, idZ);
        }
      }

      if (idXmax == iterExt[1])
      {
        // clear trailing pixels
        if (iterExt[1] < outExt[1])
        {
          setpixels(outPtr, background, outComponents, outExt[1] - iterExt[1]);
        }
        outPtr = static_cast<char*>(outPtr) + outIncY * scalarSize;

        // clear trailing rows
        if (idY == iterExt[3])
        {
          int fullspan = outExt[1] - outExt[0] + 1;
          for (idY = iterExt[3] + 1; idY <= outExt[3]; idY++)
          {
            setpixels(outPtr, background, outComponents, fullspan);
            outPtr = static_cast<char*>(outPtr) + outIncY * scalarSize;
          }
          outPtr = static_cast<char*>(outPtr) + outIncZ * scalarSize;
        }
      }
    }
  }

  // clear any trailing slices
  for (int idZ = iterExt[5] + 1; idZ <= outExt[5]; idZ++)
  {
    int fullspan = outExt[1] - outExt[0] + 1;
    for (int idY = outExt[2]; idY <= outExt[3]; idY++)
    {
      setpixels(outPtr, background, outComponents, fullspan);
      outPtr = static_cast<char*>(outPtr) + outIncY * scalarSize;
    }
    outPtr = static_cast<char*>(outPtr) + outIncZ * scalarSize;
  }

  svtkFreeBackgroundPixel(&background);

  if (doConversion)
  {
    delete[] floatPtr;
  }
  if (nsamples > 1)
  {
    delete[] floatSumPtr;
  }

  interpolator->FreePrecomputedWeights(weights);
}

//----------------------------------------------------------------------------
} // end of anonymous namespace

//----------------------------------------------------------------------------
// The transform matrix supplied by the user converts output coordinates
// to input coordinates.
// To speed up the pixel lookup, the following function provides a
// matrix which converts output pixel indices to input pixel indices.
//
// This will also concatenate the ResliceAxes and the ResliceTransform
// if possible (if the ResliceTransform is a 4x4 matrix transform).
// If it does, this->OptimizedTransform will be set to nullptr, otherwise
// this->OptimizedTransform will be equal to this->ResliceTransform.

svtkMatrix4x4* svtkImageReslice::GetIndexMatrix(svtkInformation* inInfo, svtkInformation* outInfo)
{
  // first verify that we have to update the matrix
  if (this->IndexMatrix == nullptr)
  {
    this->IndexMatrix = svtkMatrix4x4::New();
  }

  int isIdentity = 0;
  double inOrigin[3];
  double inSpacing[3];
  double outOrigin[3];
  double outSpacing[3];

  inInfo->Get(svtkDataObject::SPACING(), inSpacing);
  inInfo->Get(svtkDataObject::ORIGIN(), inOrigin);
  outInfo->Get(svtkDataObject::SPACING(), outSpacing);
  outInfo->Get(svtkDataObject::ORIGIN(), outOrigin);

  svtkTransform* transform = svtkTransform::New();
  svtkMatrix4x4* inMatrix = svtkMatrix4x4::New();
  svtkMatrix4x4* outMatrix = svtkMatrix4x4::New();

  if (this->OptimizedTransform)
  {
    this->OptimizedTransform->Delete();
  }
  this->OptimizedTransform = nullptr;

  if (this->ResliceAxes)
  {
    transform->SetMatrix(this->GetResliceAxes());
  }
  if (this->ResliceTransform)
  {
    if (this->ResliceTransform->IsA("svtkHomogeneousTransform"))
    {
      transform->PostMultiply();
      transform->Concatenate(
        static_cast<svtkHomogeneousTransform*>(this->ResliceTransform)->GetMatrix());
    }
    else
    {
      this->ResliceTransform->Register(this);
      this->OptimizedTransform = this->ResliceTransform;
    }
  }

  // check to see if we have an identity matrix
  isIdentity = svtkIsIdentityMatrix(transform->GetMatrix());

  // the outMatrix takes OutputData indices to OutputData coordinates,
  // the inMatrix takes InputData coordinates to InputData indices
  for (int i = 0; i < 3; i++)
  {
    if ((this->OptimizedTransform == nullptr &&
          (inSpacing[i] != outSpacing[i] || inOrigin[i] != outOrigin[i])) ||
      (this->OptimizedTransform != nullptr && (outSpacing[i] != 1.0 || outOrigin[i] != 0.0)))
    {
      isIdentity = 0;
    }
    inMatrix->Element[i][i] = 1.0 / inSpacing[i];
    inMatrix->Element[i][3] = -inOrigin[i] / inSpacing[i];
    outMatrix->Element[i][i] = outSpacing[i];
    outMatrix->Element[i][3] = outOrigin[i];
  }
  outInfo->Get(svtkDataObject::ORIGIN(), outOrigin);

  if (!isIdentity)
  {
    transform->PreMultiply();
    transform->Concatenate(outMatrix);
    // the OptimizedTransform requires data coords, not
    // index coords, as its input
    if (this->OptimizedTransform == nullptr)
    {
      transform->PostMultiply();
      transform->Concatenate(inMatrix);
    }
  }

  transform->GetMatrix(this->IndexMatrix);

  transform->Delete();
  inMatrix->Delete();
  outMatrix->Delete();

  return this->IndexMatrix;
}

//----------------------------------------------------------------------------
// RequestData is where the interpolator is updated, since it must be updated
// before the threads are split
int svtkImageReslice::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Generation of the StencilOutput is incompatible with splitting
  // along the x-axis when multithreaded, because of InsertNextExtent()
  if (this->GenerateStencilOutput && this->SplitPathLength == 3)
  {
    if (this->SplitMode == svtkThreadedImageAlgorithm::BLOCK)
    {
      svtkWarningMacro("RequestData: SetSplitModeToBlock() is incompatible "
                      "with GenerateStencilOutputOn().  Denying any splits "
                      "along x-axis in order to avoid corrupt stencil!");
    }
    // Ensure that x-axis is never split
    this->SplitPathLength = 2;
  }

  svtkAbstractImageInterpolator* interpolator = this->GetInterpolator();
  svtkInformation* info = inputVector[0]->GetInformationObject(0);
  interpolator->Initialize(info->Get(svtkDataObject::DATA_OBJECT()));

  int rval = this->Superclass::RequestData(request, inputVector, outputVector);

  interpolator->ReleaseData();

  return rval;
}

//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void svtkImageReslice::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** inData, svtkImageData** outData, int outExt[6], int threadId)
{
  svtkDebugMacro(<< "Execute: inData = " << inData[0][0] << ", outData = " << outData[0]);

  int inExt[6];
  inData[0][0]->GetExtent(inExt);
  // check for empty input extent
  if (inExt[1] < inExt[0] || inExt[3] < inExt[2] || inExt[5] < inExt[4])
  {
    return;
  }

  // Get the input scalars
  svtkDataArray* scalars = inData[0][0]->GetPointData()->GetScalars();

  // Get the output pointer
  void* outPtr = outData[0]->GetScalarPointerForExtent(outExt);

  // change transform matrix so that instead of taking
  // input coords -> output coords it takes output indices -> input indices
  svtkMatrix4x4* matrix = this->IndexMatrix;

  // get the portion of the transformation that remains apart from
  // the IndexMatrix
  svtkAbstractTransform* newtrans = this->OptimizedTransform;

  svtkImageResliceFloatingPointType newmat[4][4];
  for (int i = 0; i < 4; i++)
  {
    newmat[i][0] = matrix->GetElement(i, 0);
    newmat[i][1] = matrix->GetElement(i, 1);
    newmat[i][2] = matrix->GetElement(i, 2);
    newmat[i][3] = matrix->GetElement(i, 3);
  }

  if (this->HitInputExtent == 0)
  {
    svtkImageResliceClearExecute(this, outData[0], outPtr, outExt, threadId);
  }
  else if (this->UsePermuteExecute)
  {
    svtkReslicePermuteExecute(this, scalars, this->Interpolator, outData[0], outPtr,
      this->ScalarShift, this->ScalarScale,
      (this->HasConvertScalars ? &svtkImageReslice::ConvertScalarsBase : nullptr), outExt, threadId,
      newmat);
  }
  else
  {
    svtkImageResliceExecute(this, scalars, this->Interpolator, outData[0], outPtr, this->ScalarShift,
      this->ScalarScale, (this->HasConvertScalars ? &svtkImageReslice::ConvertScalarsBase : nullptr),
      outExt, threadId, newmat, newtrans);
  }
}
