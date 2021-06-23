/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageTranslateExtent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageTranslateExtent.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageTranslateExtent);

//----------------------------------------------------------------------------
svtkImageTranslateExtent::svtkImageTranslateExtent()
{
  int idx;

  for (idx = 0; idx < 3; ++idx)
  {
    this->Translation[idx] = 0;
  }
}

//----------------------------------------------------------------------------
void svtkImageTranslateExtent::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Translation: (" << this->Translation[0] << "," << this->Translation[1] << ","
     << this->Translation[2] << endl;
}

//----------------------------------------------------------------------------
// Change the WholeExtent
int svtkImageTranslateExtent::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int idx, extent[6];
  double spacing[3], origin[3];

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  inInfo->Get(svtkDataObject::ORIGIN(), origin);
  inInfo->Get(svtkDataObject::SPACING(), spacing);

  // TranslateExtent the OutputWholeExtent with the input WholeExtent
  for (idx = 0; idx < 3; ++idx)
  {
    // change extent
    extent[2 * idx] += this->Translation[idx];
    extent[2 * idx + 1] += this->Translation[idx];
    // change origin so the data does not shift
    origin[idx] -= static_cast<double>(this->Translation[idx]) * spacing[idx];
  }

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);

  return 1;
}

//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
int svtkImageTranslateExtent::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkImageData* inData = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkImageData* outData = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  int extent[6];

  // since inData can be larger than update extent.
  inData->GetExtent(extent);
  for (int i = 0; i < 3; ++i)
  {
    extent[i * 2] += this->Translation[i];
    extent[i * 2 + 1] += this->Translation[i];
  }
  outData->SetExtent(extent);
  outData->GetPointData()->PassData(inData->GetPointData());

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageTranslateExtent::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int inExtent[6], extent[6];
  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExtent);

  extent[0] = inExtent[0] - this->Translation[0];
  extent[1] = inExtent[1] - this->Translation[0];
  extent[2] = inExtent[2] - this->Translation[1];
  extent[3] = inExtent[3] - this->Translation[1];
  extent[4] = inExtent[4] - this->Translation[2];
  extent[5] = inExtent[5] - this->Translation[2];

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);

  return 1;
}
