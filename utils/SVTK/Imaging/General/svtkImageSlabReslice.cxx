/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSlabReslice.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageSlabReslice.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageSlabReslice);

//----------------------------------------------------------------------------
svtkImageSlabReslice::svtkImageSlabReslice()
{
  // Input is 3D, output is a 2D projection within the slab.
  this->OutputDimensionality = 2;

  // Number of sample points along the blendDirection to the resliced
  // direction that will be "slabbed"
  this->NumBlendSamplePoints = 1;

  // Default blend mode is maximum intensity projection through the data.
  this->BlendMode = SVTK_IMAGE_SLAB_MAX;

  this->SlabThickness = 10; // mm or world coords
  this->SlabResolution = 1; // mm or world coords
}

//----------------------------------------------------------------------------
svtkImageSlabReslice::~svtkImageSlabReslice() = default;

//----------------------------------------------------------------------------
int svtkImageSlabReslice::RequestInformation(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  this->NumBlendSamplePoints =
    2 * (static_cast<int>(this->SlabThickness / (2.0 * this->SlabResolution))) + 1;

  this->SlabNumberOfSlices = this->NumBlendSamplePoints;
  this->SlabMode = this->BlendMode;

  this->Superclass::RequestInformation(request, inputVector, outputVector);

  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  double spacing[3];
  outInfo->Get(svtkDataObject::SPACING(), spacing);
  spacing[2] = this->SlabResolution;
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);

  return 1;
}

//----------------------------------------------------------------------------
void svtkImageSlabReslice::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Blend mode: " << this->BlendMode << endl;
  os << indent << "SlabResolution (world units): " << this->SlabResolution << endl;
  os << indent << "SlabThickness (world units): " << this->SlabThickness << endl;
  os << indent << "Max Number of slices blended: " << this->NumBlendSamplePoints << endl;
}
