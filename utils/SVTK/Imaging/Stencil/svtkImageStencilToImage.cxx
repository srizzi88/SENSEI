/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageStencilToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageStencilToImage.h"

#include "svtkImageData.h"
#include "svtkImageStencilData.h"
#include "svtkImageStencilIterator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageStencilToImage);

//----------------------------------------------------------------------------
svtkImageStencilToImage::svtkImageStencilToImage()
{
  this->OutsideValue = 0;
  this->InsideValue = 1;
  this->OutputScalarType = SVTK_UNSIGNED_CHAR;

  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
svtkImageStencilToImage::~svtkImageStencilToImage() = default;

//----------------------------------------------------------------------------
int svtkImageStencilToImage::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int extent[6];
  double spacing[3];
  double origin[3];

  inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  inInfo->Get(svtkDataObject::SPACING(), spacing);
  inInfo->Get(svtkDataObject::ORIGIN(), origin);

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
  outInfo->Set(svtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);

  svtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, -1);

  return 1;
}

//----------------------------------------------------------------------------
template <class T>
void svtkImageStencilToImageExecute(svtkImageStencilToImage* self, svtkImageStencilData* stencil,
  svtkImageData* outData, T*, int outExt[6], int id)
{
  double inValueD = self->GetInsideValue();
  double outValueD = self->GetOutsideValue();

  double tmin = outData->GetScalarTypeMin();
  double tmax = outData->GetScalarTypeMax();

  if (inValueD < tmin)
  {
    inValueD = tmin;
  }
  if (inValueD > tmax)
  {
    inValueD = tmax;
  }
  if (outValueD < tmin)
  {
    outValueD = tmin;
  }
  if (outValueD > tmax)
  {
    outValueD = tmax;
  }

  T inValue = static_cast<T>(inValueD);
  T outValue = static_cast<T>(outValueD);

  svtkImageStencilIterator<T> outIter(outData, stencil, outExt, self, id);

  // Loop through output pixels
  while (!outIter.IsAtEnd())
  {
    T* outPtr = outIter.BeginSpan();
    T* spanEndPtr = outIter.EndSpan();

    if (outIter.IsInStencil())
    {
      while (outPtr != spanEndPtr)
      {
        *outPtr++ = inValue;
      }
    }
    else
    {
      while (outPtr != spanEndPtr)
      {
        *outPtr++ = outValue;
      }
    }

    outIter.NextSpan();
  }
}

//----------------------------------------------------------------------------
int svtkImageStencilToImage::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  int updateExtent[6];
  outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), updateExtent);
  svtkImageData* outData = static_cast<svtkImageData*>(outInfo->Get(svtkDataObject::DATA_OBJECT()));
  this->AllocateOutputData(outData, outInfo, updateExtent);
  void* outPtr = outData->GetScalarPointerForExtent(updateExtent);

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkImageStencilData* inData =
    static_cast<svtkImageStencilData*>(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  switch (outData->GetScalarType())
  {
    svtkTemplateMacro(svtkImageStencilToImageExecute(
      this, inData, outData, static_cast<SVTK_TT*>(outPtr), updateExtent, 0));
    default:
      svtkErrorMacro("Execute: Unknown ScalarType");
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkImageStencilToImage::FillInputPortInformation(int port, svtkInformation* info)
{
  if (port == 0)
  {
    info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageStencilData");
    info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  }
  return 1;
}

//----------------------------------------------------------------------------
void svtkImageStencilToImage::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InsideValue: " << this->InsideValue << "\n";
  os << indent << "OutsideValue: " << this->OutsideValue << "\n";
  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
}
