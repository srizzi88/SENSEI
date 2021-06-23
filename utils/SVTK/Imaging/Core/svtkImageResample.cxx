/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageResample.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageResample.h"

#include "svtkAlgorithmOutput.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageResample);

//----------------------------------------------------------------------------
// Constructor: Sets default filter to be identity.
svtkImageResample::svtkImageResample()
{
  this->MagnificationFactors[0] = 1.0;
  this->MagnificationFactors[1] = 1.0;
  this->MagnificationFactors[2] = 1.0;
  this->OutputSpacing[0] = 0.0; // not specified
  this->OutputSpacing[1] = 0.0; // not specified
  this->OutputSpacing[2] = 0.0; // not specified
  this->InterpolationMode = SVTK_RESLICE_LINEAR;
  this->Dimensionality = 3;
}

//----------------------------------------------------------------------------
void svtkImageResample::SetOutputSpacing(double sx, double sy, double sz)
{
  const double spacing[3] = { sx, sy, sz };
  bool modified = false;

  for (int axis = 0; axis < 3; axis++)
  {
    if (this->OutputSpacing[axis] != spacing[axis])
    {
      this->OutputSpacing[axis] = spacing[axis];
      if (spacing[axis] != 0.0)
      {
        // Delay computing the magnification factor.
        // Input might not be set yet.
        this->MagnificationFactors[axis] = 0.0; // Not computed yet.
      }
      modified = true;
    }
  }

  if (modified)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkImageResample::SetAxisOutputSpacing(int axis, double s)
{
  if (axis < 0 || axis > 2)
  {
    svtkErrorMacro("Bad axis: " << axis);
    return;
  }

  double spacing[3];
  this->GetOutputSpacing(spacing);
  spacing[axis] = s;

  this->SetOutputSpacing(spacing);
}

//----------------------------------------------------------------------------
void svtkImageResample::SetMagnificationFactors(double fx, double fy, double fz)
{
  const double factors[3] = { fx, fy, fz };
  bool modified = false;

  for (int axis = 0; axis < 3; axis++)
  {
    if (this->MagnificationFactors[axis] != factors[axis])
    {
      this->MagnificationFactors[axis] = factors[axis];
      // Spacing is no longer valid.
      this->OutputSpacing[axis] = 0.0; // Not computed yet.
      modified = true;
    }
  }

  if (modified)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkImageResample::SetAxisMagnificationFactor(int axis, double factor)
{
  if (axis < 0 || axis > 2)
  {
    svtkErrorMacro("Bad axis: " << axis);
    return;
  }

  double factors[3];
  this->GetMagnificationFactors(factors);
  factors[axis] = factor;

  this->SetMagnificationFactors(factors);
}

//----------------------------------------------------------------------------
double svtkImageResample::GetAxisMagnificationFactor(int axis, svtkInformation* inInfo)
{
  if (axis < 0 || axis > 2)
  {
    svtkErrorMacro("Bad axis: " << axis);
    return 0.0;
  }

  if (this->MagnificationFactors[axis] == 0.0)
  {
    double* inputSpacing;
    if (!this->GetInput())
    {
      svtkErrorMacro("GetMagnificationFactor: Input not set.");
      return 0.0;
    }
    this->GetInputConnection(0, 0)->GetProducer()->UpdateInformation();
    if (!inInfo)
    {
      inInfo = this->GetExecutive()->GetInputInformation(0, 0);
    }
    inputSpacing = inInfo->Get(svtkDataObject::SPACING());
    this->MagnificationFactors[axis] = inputSpacing[axis] / this->OutputSpacing[axis];
  }

  svtkDebugMacro(
    "Returning magnification factor " << this->MagnificationFactors[axis] << " for axis " << axis);

  return this->MagnificationFactors[axis];
}

//----------------------------------------------------------------------------
// Computes any global image information associated with regions.
int svtkImageResample::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int wholeMin, wholeMax, axis, ext[6];
  double spacing[3], factor;

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext);
  inInfo->Get(svtkDataObject::SPACING(), spacing);

  for (axis = 0; axis < 3; axis++)
  {
    wholeMin = ext[axis * 2];
    wholeMax = ext[axis * 2 + 1];

    // Scale the output extent
    factor = 1.0;
    if (axis < this->Dimensionality)
    {
      factor = this->GetAxisMagnificationFactor(axis, inInfo);
    }

    wholeMin = static_cast<int>(ceil(static_cast<double>(wholeMin) * factor));
    wholeMax = static_cast<int>(floor(static_cast<double>(wholeMax) * factor));

    // Change the data spacing
    spacing[axis] /= factor;

    ext[axis * 2] = wholeMin;
    ext[axis * 2 + 1] = wholeMax;

    // just in case the input spacing has changed.
    if (this->OutputSpacing[axis] != 0.0)
    {
      // Cause MagnificationFactor to recompute.
      this->MagnificationFactors[axis] = 0.0;
    }
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), ext, 6);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);

  return 1;
}

void svtkImageResample::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MagnificationFactors: " << this->MagnificationFactors[0] << " "
     << this->MagnificationFactors[1] << " " << this->MagnificationFactors[2] << "\n";
  os << indent << "Dimensionality: " << this->Dimensionality << "\n";
  os << indent << "Interpolate: " << (this->GetInterpolate() ? "On\n" : "Off\n");
}
