/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageFFT.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImageFFT.h"

#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkStreamingDemandDrivenPipeline.h"

#include <cmath>

svtkStandardNewMacro(svtkImageFFT);

//----------------------------------------------------------------------------
// This extent of the components changes to real and imaginary values.
int svtkImageFFT::IterativeRequestInformation(
  svtkInformation* svtkNotUsed(input), svtkInformation* output)
{
  svtkDataObject::SetPointDataActiveScalarInfo(output, SVTK_DOUBLE, 2);
  return 1;
}

//----------------------------------------------------------------------------
static void svtkImageFFTInternalRequestUpdateExtent(
  int* inExt, int* outExt, int* wExt, int iteration)
{
  memcpy(inExt, outExt, 6 * sizeof(int));
  inExt[iteration * 2] = wExt[iteration * 2];
  inExt[iteration * 2 + 1] = wExt[iteration * 2 + 1];
}

//----------------------------------------------------------------------------
// This method tells the superclass that the whole input array is needed
// to compute any output region.
int svtkImageFFT::IterativeRequestUpdateExtent(svtkInformation* input, svtkInformation* output)
{
  int* outExt = output->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  int* wExt = input->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  int inExt[6];
  svtkImageFFTInternalRequestUpdateExtent(inExt, outExt, wExt, this->Iteration);
  input->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), inExt, 6);

  return 1;
}

//----------------------------------------------------------------------------
// This templated execute method handles any type input, but the output
// is always doubles.
template <class T>
void svtkImageFFTExecute(svtkImageFFT* self, svtkImageData* inData, int inExt[6], T* inPtr,
  svtkImageData* outData, int outExt[6], double* outPtr, int id)
{
  svtkImageComplex* inComplex;
  svtkImageComplex* outComplex;
  svtkImageComplex* pComplex;
  //
  int inMin0, inMax0;
  svtkIdType inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  //
  int outMin0, outMax0, outMin1, outMax1, outMin2, outMax2;
  svtkIdType outInc0, outInc1, outInc2;
  double *outPtr0, *outPtr1, *outPtr2;
  //
  int idx0, idx1, idx2, inSize0, numberOfComponents;
  unsigned long count = 0;
  unsigned long target;
  double startProgress;

  startProgress = self->GetIteration() / static_cast<double>(self->GetNumberOfIterations());

  // Reorder axes (The outs here are just placeholders)
  self->PermuteExtent(inExt, inMin0, inMax0, outMin1, outMax1, outMin2, outMax2);
  self->PermuteExtent(outExt, outMin0, outMax0, outMin1, outMax1, outMin2, outMax2);
  self->PermuteIncrements(inData->GetIncrements(), inInc0, inInc1, inInc2);
  self->PermuteIncrements(outData->GetIncrements(), outInc0, outInc1, outInc2);

  inSize0 = inMax0 - inMin0 + 1;

  // Input has to have real components at least.
  numberOfComponents = inData->GetNumberOfScalarComponents();
  if (numberOfComponents < 1)
  {
    svtkGenericWarningMacro("No real components");
    return;
  }

  // Allocate the arrays of complex numbers
  inComplex = new svtkImageComplex[inSize0];
  outComplex = new svtkImageComplex[inSize0];

  target = static_cast<unsigned long>(
    (outMax2 - outMin2 + 1) * (outMax1 - outMin1 + 1) * self->GetNumberOfIterations() / 50.0);
  target++;

  // loop over other axes
  inPtr2 = inPtr;
  outPtr2 = outPtr;
  for (idx2 = outMin2; idx2 <= outMax2; ++idx2)
  {
    inPtr1 = inPtr2;
    outPtr1 = outPtr2;
    for (idx1 = outMin1; !self->AbortExecute && idx1 <= outMax1; ++idx1)
    {
      if (!id)
      {
        if (!(count % target))
        {
          self->UpdateProgress(count / (50.0 * target) + startProgress);
        }
        count++;
      }
      // copy into complex numbers
      inPtr0 = inPtr1;
      pComplex = inComplex;
      for (idx0 = inMin0; idx0 <= inMax0; ++idx0)
      {
        pComplex->Real = static_cast<double>(*inPtr0);
        pComplex->Imag = 0.0;
        if (numberOfComponents > 1)
        { // yes we have an imaginary input
          pComplex->Imag = static_cast<double>(inPtr0[1]);
        }
        inPtr0 += inInc0;
        ++pComplex;
      }

      // Call the method that performs the fft
      self->ExecuteFft(inComplex, outComplex, inSize0);

      // copy into output
      outPtr0 = outPtr1;
      pComplex = outComplex + (outMin0 - inMin0);
      for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
      {
        *outPtr0 = static_cast<double>(pComplex->Real);
        outPtr0[1] = static_cast<double>(pComplex->Imag);
        outPtr0 += outInc0;
        ++pComplex;
      }
      inPtr1 += inInc1;
      outPtr1 += outInc1;
    }
    inPtr2 += inInc2;
    outPtr2 += outInc2;
  }

  delete[] inComplex;
  delete[] outComplex;
}

//----------------------------------------------------------------------------
// This method is passed input and output Datas, and executes the fft
// algorithm to fill the output from the input.
// Not threaded yet.
void svtkImageFFT::ThreadedRequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector),
  svtkImageData*** inDataVec, svtkImageData** outDataVec, int outExt[6], int threadId)
{
  svtkImageData* inData = inDataVec[0][0];
  svtkImageData* outData = outDataVec[0];
  void *inPtr, *outPtr;
  int inExt[6];
  int* wExt =
    inputVector[0]->GetInformationObject(0)->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  svtkImageFFTInternalRequestUpdateExtent(inExt, outExt, wExt, this->Iteration);

  inPtr = inData->GetScalarPointerForExtent(inExt);
  outPtr = outData->GetScalarPointerForExtent(outExt);

  // this filter expects that the output be doubles.
  if (outData->GetScalarType() != SVTK_DOUBLE)
  {
    svtkErrorMacro(<< "Execute: Output must be type double.");
    return;
  }

  // this filter expects input to have 1 or two components
  if (outData->GetNumberOfScalarComponents() != 1 && outData->GetNumberOfScalarComponents() != 2)
  {
    svtkErrorMacro(<< "Execute: Cannot handle more than 2 components");
    return;
  }

  // choose which templated function to call.
  switch (inData->GetScalarType())
  {
    svtkTemplateMacro(svtkImageFFTExecute(this, inData, inExt, static_cast<SVTK_TT*>(inPtr), outData,
      outExt, static_cast<double*>(outPtr), threadId));
    default:
      svtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
  }
}
