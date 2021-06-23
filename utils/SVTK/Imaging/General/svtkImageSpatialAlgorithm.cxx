/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSpatialAlgorithm.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageSpatialAlgorithm.h"

#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkImageSpatialAlgorithm);

//----------------------------------------------------------------------------
// Construct an instance of svtkImageSpatialAlgorithm filter.
svtkImageSpatialAlgorithm::svtkImageSpatialAlgorithm()
{
  this->KernelSize[0] = this->KernelSize[1] = this->KernelSize[2] = 1;
  this->KernelMiddle[0] = this->KernelMiddle[1] = this->KernelMiddle[2] = 0;
  this->HandleBoundaries = 1;
}

//----------------------------------------------------------------------------
void svtkImageSpatialAlgorithm::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  int idx;

  os << indent << "KernelSize: (" << this->KernelSize[0];
  for (idx = 1; idx < 3; ++idx)
  {
    os << ", " << this->KernelSize[idx];
  }
  os << ").\n";

  os << indent << "KernelMiddle: (" << this->KernelMiddle[0];
  for (idx = 1; idx < 3; ++idx)
  {
    os << ", " << this->KernelMiddle[idx];
  }
  os << ").\n";
}

//----------------------------------------------------------------------------
int svtkImageSpatialAlgorithm::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Take this opportunity to override the defaults.
  int extent[6];
  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  this->ComputeOutputWholeExtent(extent, this->HandleBoundaries);
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);

  return 1;
}

//----------------------------------------------------------------------------
// A helper method to compute output image extent
void svtkImageSpatialAlgorithm::ComputeOutputWholeExtent(int extent[6], int handleBoundaries)
{
  int idx;

  if (!handleBoundaries)
  {
    // Make extent a little smaller because of the kernel size.
    for (idx = 0; idx < 3; ++idx)
    {
      extent[idx * 2] += this->KernelMiddle[idx];
      extent[idx * 2 + 1] -= (this->KernelSize[idx] - 1) - this->KernelMiddle[idx];
    }
  }
}

//----------------------------------------------------------------------------
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the
// extent of the output region.  After this method finishes, "region" should
// have the extent of the required input region.
int svtkImageSpatialAlgorithm::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  int wholeExtent[6], extent[6], inExtent[6];

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExtent);

  this->InternalRequestUpdateExtent(extent, inExtent, wholeExtent);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);

  return 1;
}

//----------------------------------------------------------------------------
void svtkImageSpatialAlgorithm::InternalRequestUpdateExtent(
  int* extent, int* inExtent, int* wholeExtent)
{
  int idx;
  for (idx = 0; idx < 3; ++idx)
  {
    // Magnify by strides
    extent[idx * 2] = inExtent[idx * 2];
    extent[idx * 2 + 1] = inExtent[idx * 2 + 1];

    // Expand to get inRegion Extent
    extent[idx * 2] -= this->KernelMiddle[idx];
    extent[idx * 2 + 1] += (this->KernelSize[idx] - 1) - this->KernelMiddle[idx];

    // If the expanded region is out of the IMAGE Extent (grow min)
    if (extent[idx * 2] < wholeExtent[idx * 2])
    {
      if (this->HandleBoundaries)
      {
        // shrink the required region extent
        extent[idx * 2] = wholeExtent[idx * 2];
      }
      else
      {
        svtkWarningMacro(<< "Required region is out of the image extent.");
      }
    }
    // If the expanded region is out of the IMAGE Extent (shrink max)
    if (extent[idx * 2 + 1] > wholeExtent[idx * 2 + 1])
    {
      if (this->HandleBoundaries)
      {
        // shrink the required region extent
        extent[idx * 2 + 1] = wholeExtent[idx * 2 + 1];
      }
      else
      {
        svtkWarningMacro(<< "Required region is out of the image extent.");
      }
    }
  }
}
