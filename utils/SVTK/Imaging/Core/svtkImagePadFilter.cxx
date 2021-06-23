/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImagePadFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImagePadFilter.h"

#include "svtkDataSetAttributes.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImagePadFilter);

//----------------------------------------------------------------------------
// Constructor sets default values
svtkImagePadFilter::svtkImagePadFilter()
{
  int idx;

  // Initialize output image extent to INVALID
  for (idx = 0; idx < 3; ++idx)
  {
    this->OutputWholeExtent[idx * 2] = 0;
    this->OutputWholeExtent[idx * 2 + 1] = -1;
  }
  // Set Output numberOfScalarComponents to INVALID
  this->OutputNumberOfScalarComponents = -1;
}

//----------------------------------------------------------------------------
void svtkImagePadFilter::SetOutputWholeExtent(int extent[6])
{
  int idx, modified = 0;

  for (idx = 0; idx < 6; ++idx)
  {
    if (this->OutputWholeExtent[idx] != extent[idx])
    {
      this->OutputWholeExtent[idx] = extent[idx];
      modified = 1;
    }
  }

  if (modified)
  {
    this->Modified();
  }
}
//----------------------------------------------------------------------------
void svtkImagePadFilter::SetOutputWholeExtent(
  int minX, int maxX, int minY, int maxY, int minZ, int maxZ)
{
  int extent[6];

  extent[0] = minX;
  extent[1] = maxX;
  extent[2] = minY;
  extent[3] = maxY;
  extent[4] = minZ;
  extent[5] = maxZ;
  this->SetOutputWholeExtent(extent);
}

//----------------------------------------------------------------------------
void svtkImagePadFilter::GetOutputWholeExtent(int extent[6])
{
  int idx;

  for (idx = 0; idx < 6; ++idx)
  {
    extent[idx] = this->OutputWholeExtent[idx];
  }
}

//----------------------------------------------------------------------------
// Just change the Image extent.
int svtkImagePadFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  if (this->OutputWholeExtent[0] > this->OutputWholeExtent[1])
  {
    // invalid setting, it has not been set, so default to whole Extent
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->OutputWholeExtent);
  }
  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->OutputWholeExtent, 6);

  if (this->OutputNumberOfScalarComponents < 0)
  {
    svtkInformation* inScalarInfo = svtkDataObject::GetActiveFieldInformation(
      inInfo, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
    if (!inScalarInfo)
    {
      svtkErrorMacro("Missing scalar field on input information!");
      return 0;
    }
    // invalid setting, it has not been set, so default to input.
    this->OutputNumberOfScalarComponents =
      inScalarInfo->Get(svtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
  }
  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, -1, this->OutputNumberOfScalarComponents);
  return 1;
}

void svtkImagePadFilter::ComputeInputUpdateExtent(int inExt[6], int outExt[6], int wholeExtent[6])
{
  int idx;

  // Clip
  for (idx = 0; idx < 3; ++idx)
  {
    inExt[idx * 2] = outExt[idx * 2];
    inExt[idx * 2 + 1] = outExt[idx * 2 + 1];
    if (inExt[idx * 2] < wholeExtent[idx * 2])
    {
      inExt[idx * 2] = wholeExtent[idx * 2];
    }
    if (inExt[idx * 2] > wholeExtent[idx * 2 + 1])
    {
      inExt[idx * 2] = wholeExtent[idx * 2 + 1];
    }
    if (inExt[idx * 2 + 1] < wholeExtent[idx * 2])
    {
      inExt[idx * 2 + 1] = wholeExtent[idx * 2];
    }
    if (inExt[idx * 2 + 1] > wholeExtent[idx * 2 + 1])
    {
      inExt[idx * 2 + 1] = wholeExtent[idx * 2 + 1];
    }
  }
}

//----------------------------------------------------------------------------
// Just clip the request.  The subclass may need to overwrite this method.
int svtkImagePadFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int wholeExtent[6];
  int inExt[6];

  // handle XYZ
  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt);

  this->ComputeInputUpdateExtent(inExt, inExt, wholeExtent);

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  return 1;
}

void svtkImagePadFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "OutputNumberOfScalarComponents: " << this->OutputNumberOfScalarComponents
     << "\n";
}
