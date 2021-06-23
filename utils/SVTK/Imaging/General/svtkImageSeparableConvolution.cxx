/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSeparableConvolution.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageSeparableConvolution.h"

#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkImageSeparableConvolution);
svtkCxxSetObjectMacro(svtkImageSeparableConvolution, XKernel, svtkFloatArray);
svtkCxxSetObjectMacro(svtkImageSeparableConvolution, YKernel, svtkFloatArray);
svtkCxxSetObjectMacro(svtkImageSeparableConvolution, ZKernel, svtkFloatArray);

// Actually do the convolution
static void ExecuteConvolve(
  float* kernel, int kernelSize, float* image, float* outImage, int imageSize)
{

  // Consider the kernel to be centered at (int) ( (kernelSize - 1 ) / 2.0 )

  int center = static_cast<int>((kernelSize - 1) / 2.0);
  int i, j, k, kStart, iStart, iEnd, count;

  for (i = 0; i < imageSize; ++i)
  {
    outImage[i] = 0.0;

    //    iStart = i - center;
    //    if ( iStart < 0 )
    //      {
    //      iStart = 0;
    //      }

    //    iEnd = i + center;
    //    if ( iEnd > imageSize - 1 )
    //      {
    //      iEnd = imageSize - 1;
    //      }

    // Handle padding
    iStart = i - center;
    k = kernelSize - 1;
    while (iStart < 0)
    {
      outImage[i] += image[0] * kernel[k];
      ++iStart;
      --k;
    }

    iEnd = i + center;
    k = 0;
    while (iEnd > imageSize - 1)
    {
      outImage[i] += image[imageSize - 1] * kernel[k];
      ++k;
      --iEnd;
    }

    kStart = center + i;
    if (kStart > kernelSize - 1)
    {
      kStart = kernelSize - 1;
    }
    count = iEnd - iStart + 1;
    for (j = 0; j < count; ++j)
    {
      outImage[i] += image[j + iStart] * kernel[kStart - j];
    }
  }
}

// Description:
// Overload standard modified time function. If kernel arrays are modified,
// then this object is modified as well.
svtkMTimeType svtkImageSeparableConvolution::GetMTime()
{
  svtkMTimeType mTime = this->svtkImageDecomposeFilter::GetMTime();
  svtkMTimeType kTime;

  if (this->XKernel)
  {
    kTime = this->XKernel->GetMTime();
    mTime = kTime > mTime ? kTime : mTime;
  }
  if (this->YKernel)
  {
    kTime = this->YKernel->GetMTime();
    mTime = kTime > mTime ? kTime : mTime;
  }
  if (this->YKernel)
  {
    kTime = this->YKernel->GetMTime();
    mTime = kTime > mTime ? kTime : mTime;
  }
  return mTime;
}

//----------------------------------------------------------------------------
svtkImageSeparableConvolution::~svtkImageSeparableConvolution()
{
  if (this->XKernel)
  {
    this->XKernel->UnRegister(this);
  }
  if (this->YKernel)
  {
    this->YKernel->UnRegister(this);
  }
  if (this->ZKernel)
  {
    this->ZKernel->UnRegister(this);
  }
}

//----------------------------------------------------------------------------
svtkImageSeparableConvolution::svtkImageSeparableConvolution()
{
  XKernel = YKernel = ZKernel = nullptr;
}

//----------------------------------------------------------------------------
// This extent of the components changes to real and imaginary values.
int svtkImageSeparableConvolution::IterativeRequestInformation(
  svtkInformation* svtkNotUsed(input), svtkInformation* output)
{
  svtkDataObject::SetPointDataActiveScalarInfo(output, SVTK_FLOAT, 1);
  return 1;
}

//----------------------------------------------------------------------------
// This method tells the superclass that the whole input array is needed
// to compute any output region.

int svtkImageSeparableConvolution::IterativeRequestUpdateExtent(
  svtkInformation* input, svtkInformation* output)
{
  int* wholeExtent = input->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  svtkFloatArray* KernelArray = nullptr;
  switch (this->GetIteration())
  {
    case 0:
      KernelArray = this->GetXKernel();
      break;
    case 1:
      KernelArray = this->GetYKernel();
      break;
    case 2:
      KernelArray = this->GetZKernel();
      break;
  }
  int kernelSize = 0;
  if (KernelArray)
  {
    kernelSize = KernelArray->GetNumberOfTuples();
    kernelSize = static_cast<int>((kernelSize - 1) / 2.0);
  }

  int* outExt = output->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());

  // Assumes that the input update extent has been initialized to output ...
  int inExt[6];
  memcpy(inExt, outExt, 6 * sizeof(int));
  inExt[this->Iteration * 2] = outExt[this->Iteration * 2] - kernelSize;
  if (inExt[this->Iteration * 2] < wholeExtent[this->Iteration * 2])
  {
    inExt[this->Iteration * 2] = wholeExtent[this->Iteration * 2];
  }

  inExt[this->Iteration * 2 + 1] = outExt[this->Iteration * 2 + 1] + kernelSize;
  if (inExt[this->Iteration * 2 + 1] > wholeExtent[this->Iteration * 2 + 1])
  {
    inExt[this->Iteration * 2 + 1] = wholeExtent[this->Iteration * 2 + 1];
  }

  input->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  return 1;
}

template <class T>
void svtkImageSeparableConvolutionExecute(svtkImageSeparableConvolution* self, svtkImageData* inData,
  svtkImageData* outData, T* svtkNotUsed(dummy), int* inExt, int* outExt)
{
  T *inPtr0, *inPtr1, *inPtr2;
  float *outPtr0, *outPtr1, *outPtr2;
  svtkIdType inInc0, inInc1, inInc2;
  svtkIdType outInc0, outInc1, outInc2;
  int inMin0, inMax0, inMin1, inMax1, inMin2, inMax2;
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  int idx0, idx1, idx2;
  int i;
  unsigned long count = 0;
  unsigned long target;

  // Reorder axes (the in and out extents are assumed to be the same)
  // (see intercept cache update)
  self->PermuteExtent(outExt, outMin0, outMax0, outMin1, outMax1, outMin2, outMax2);
  self->PermuteExtent(inExt, inMin0, inMax0, inMin1, inMax1, inMin2, inMax2);
  self->PermuteIncrements(inData->GetIncrements(), inInc0, inInc1, inInc2);
  self->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);

  target = static_cast<unsigned long>((inMax2 - inMin2 + 1) * (inMax1 - inMin1 + 1) / 50.0);
  target++;

  svtkFloatArray* KernelArray = nullptr;
  switch (self->GetIteration())
  {
    case 0:
      KernelArray = self->GetXKernel();
      break;
    case 1:
      KernelArray = self->GetYKernel();
      break;
    case 2:
      KernelArray = self->GetZKernel();
      break;
  }
  int kernelSize = 0;
  float* kernel = nullptr;

  if (KernelArray)
  {
    // Allocate the arrays
    kernelSize = KernelArray->GetNumberOfTuples();
    kernel = new float[kernelSize];
    // Copy the kernel
    for (i = 0; i < kernelSize; i++)
    {
      kernel[i] = KernelArray->GetValue(i);
    }
  }

  int imageSize = inMax0 + 1;
  float* image = new float[imageSize];
  float* outImage = new float[imageSize];
  float* imagePtr;

  // loop over all the extra axes
  inPtr2 = static_cast<T*>(inData->GetScalarPointerForExtent(inExt));
  outPtr2 = static_cast<float*>(outData->GetScalarPointerForExtent(outExt));
  for (idx2 = inMin2; idx2 <= inMax2; ++idx2)
  {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = inMin1; !self->AbortExecute && idx1 <= inMax1; ++idx1)
    {
      if (!(count % target))
      {
        self->UpdateProgress(count / (50.0 * target));
      }
      count++;
      inPtr0 = inPtr1;
      imagePtr = image;
      for (idx0 = inMin0; idx0 <= inMax0; ++idx0)
      {
        *imagePtr = static_cast<float>(*inPtr0);
        inPtr0 += inInc0;
        ++imagePtr;
      }

      // Call the method that performs the convolution
      if (kernel)
      {
        ExecuteConvolve(kernel, kernelSize, image, outImage, imageSize);
        imagePtr = outImage;
      }
      else
      {
        // If we don't have a kernel, just copy to the output
        imagePtr = image;
      }

      // Copy to output, be aware that we only copy to the extent that was asked for
      outPtr0 = outPtr1;
      imagePtr = imagePtr + (outMin0 - inMin0);
      for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
      {
        *outPtr0 = (*imagePtr);
        outPtr0 += outInc0;
        ++imagePtr;
      }
      inPtr1 += inInc1;
      outPtr1 += outInc1;
    }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
  }

  delete[] image;
  delete[] outImage;
  delete[] kernel;
}

//----------------------------------------------------------------------------
// This is written as a 1D execute method, but is called several times.
int svtkImageSeparableConvolution::IterativeRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkImageData* inData = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  svtkImageData* outData = svtkImageData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  outData->SetExtent(outInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  outData->AllocateScalars(outInfo);

  if (XKernel)
  {
    // Check for a filter of odd length
    if (1 - (XKernel->GetNumberOfTuples() % 2))
    {
      svtkErrorMacro(<< "Execute:  XKernel must have odd length");
      return 1;
    }
  }
  if (YKernel)
  {
    // Check for a filter of odd length
    if (1 - (YKernel->GetNumberOfTuples() % 2))
    {
      svtkErrorMacro(<< "Execute:  YKernel must have odd length");
      return 1;
    }
  }
  if (ZKernel)
  {
    // Check for a filter of odd length
    if (1 - (ZKernel->GetNumberOfTuples() % 2))
    {
      svtkErrorMacro(<< "Execute:  ZKernel must have odd length");
      return 1;
    }
  }

  if (inData->GetNumberOfScalarComponents() != 1)
  {
    svtkErrorMacro(<< "ImageSeparableConvolution only works on 1 component input for the moment.");
    return 1;
  }

  // this filter expects that the output be floats.
  if (outData->GetScalarType() != SVTK_FLOAT)
  {
    svtkErrorMacro(<< "Execute: Output must be type float.");
    return 1;
  }

  // choose which templated function to call.
  switch (inData->GetScalarType())
  {
    svtkTemplateMacro(svtkImageSeparableConvolutionExecute(this, inData, outData,
      static_cast<SVTK_TT*>(nullptr), inInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT())));
    default:
      svtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 1;
  }

  return 1;
}

void svtkImageSeparableConvolution::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->XKernel)
  {
    os << indent << "XKernel:\n";
    this->XKernel->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "XKernel: (not defined)\n";
  }
  if (this->YKernel)
  {
    os << indent << "YKernel:\n";
    this->YKernel->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "YKernel: (not defined)\n";
  }
  if (this->ZKernel)
  {
    os << indent << "ZKernel:\n";
    this->ZKernel->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "ZKernel: (not defined)\n";
  }
}
