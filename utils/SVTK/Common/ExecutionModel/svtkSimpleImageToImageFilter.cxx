/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimpleImageToImageFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSimpleImageToImageFilter.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
svtkSimpleImageToImageFilter::svtkSimpleImageToImageFilter() = default;

//----------------------------------------------------------------------------
svtkSimpleImageToImageFilter::~svtkSimpleImageToImageFilter() = default;

//----------------------------------------------------------------------------
int svtkSimpleImageToImageFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // always request the whole extent
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  return 1;
}

//----------------------------------------------------------------------------
int svtkSimpleImageToImageFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the data object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* output = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkImageData* input = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  int inExt[6];
  input->GetExtent(inExt);
  // if the input extent is empty then exit
  if (inExt[1] < inExt[0] || inExt[3] < inExt[2] || inExt[5] < inExt[4])
  {
    return 1;
  }

  // Set the extent of the output and allocate memory.
  output->SetExtent(outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  output->AllocateScalars(outInfo);

  this->SimpleExecute(input, output);

  return 1;
}

//----------------------------------------------------------------------------
void svtkSimpleImageToImageFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
