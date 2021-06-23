/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageInPlaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageInPlaceFilter.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLargeInteger.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
svtkImageInPlaceFilter::svtkImageInPlaceFilter() = default;

//----------------------------------------------------------------------------
svtkImageInPlaceFilter::~svtkImageInPlaceFilter() = default;

//----------------------------------------------------------------------------

int svtkImageInPlaceFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the data object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* output = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkImageData* input = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  int *inExt, *outExt;
  inExt = inInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  outExt = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  // if the total size of the data is the same then can be in place
  svtkLargeInteger inSize;
  svtkLargeInteger outSize;
  inSize = (inExt[1] - inExt[0] + 1);
  inSize = inSize * (inExt[3] - inExt[2] + 1);
  inSize = inSize * (inExt[5] - inExt[4] + 1);
  outSize = (outExt[1] - outExt[0] + 1);
  outSize = outSize * (outExt[3] - outExt[2] + 1);
  outSize = outSize * (outExt[5] - outExt[4] + 1);
  if (inSize == outSize &&
    (svtkDataObject::GetGlobalReleaseDataFlag() ||
      inInfo->Get(svtkDemandDrivenPipeline::RELEASE_DATA())))
  {
    // pass the data
    output->GetPointData()->PassData(input->GetPointData());
    output->SetExtent(outExt);
  }
  else
  {
    output->SetExtent(outExt);
    output->AllocateScalars(outInfo);
    this->CopyData(input, output, outExt);
  }

  return 1;
}

void svtkImageInPlaceFilter::CopyData(svtkImageData* inData, svtkImageData* outData, int* outExt)
{
  char* inPtr = static_cast<char*>(inData->GetScalarPointerForExtent(outExt));
  char* outPtr = static_cast<char*>(outData->GetScalarPointerForExtent(outExt));
  int rowLength, size;
  svtkIdType inIncX, inIncY, inIncZ;
  svtkIdType outIncX, outIncY, outIncZ;
  int idxY, idxZ, maxY, maxZ;

  rowLength = (outExt[1] - outExt[0] + 1) * inData->GetNumberOfScalarComponents();
  size = inData->GetScalarSize();
  rowLength *= size;
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];

  // Get increments to march through data
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // adjust increments for this loop
  inIncY = inIncY * size + rowLength;
  outIncY = outIncY * size + rowLength;
  inIncZ *= size;
  outIncZ *= size;

  // Loop through output pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
  {
    for (idxY = 0; idxY <= maxY; idxY++)
    {
      memcpy(outPtr, inPtr, rowLength);
      outPtr += outIncY;
      inPtr += inIncY;
    }
    outPtr += outIncZ;
    inPtr += inIncZ;
  }
}

//----------------------------------------------------------------------------
void svtkImageInPlaceFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
