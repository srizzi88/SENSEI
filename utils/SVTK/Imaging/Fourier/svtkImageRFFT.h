/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageRFFT.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageRFFT
 * @brief    Reverse Fast Fourier Transform.
 *
 * svtkImageRFFT implements the reverse fast Fourier transform.  The input
 * can have real or complex data in any components and data types, but
 * the output is always complex doubles with real values in component0, and
 * imaginary values in component1.  The filter is fastest for images that
 * have power of two sizes.  The filter uses butterfly filters for each
 * prime factor of the dimension.  This makes images with prime number dimensions
 * (i.e. 17x17) much slower to compute.  Multi dimensional (i.e volumes)
 * FFT's are decomposed so that each axis executes in series.
 * In most cases the RFFT will produce an image whose imaginary values are all
 * zero's. In this case svtkImageExtractComponents can be used to remove
 * this imaginary components leaving only the real image.
 *
 * @sa
 * svtkImageExtractComponenents
 */

#ifndef svtkImageRFFT_h
#define svtkImageRFFT_h

#include "svtkImageFourierFilter.h"
#include "svtkImagingFourierModule.h" // For export macro

class SVTKIMAGINGFOURIER_EXPORT svtkImageRFFT : public svtkImageFourierFilter
{
public:
  static svtkImageRFFT* New();
  svtkTypeMacro(svtkImageRFFT, svtkImageFourierFilter);

protected:
  svtkImageRFFT() {}
  ~svtkImageRFFT() override {}

  int IterativeRequestInformation(svtkInformation* in, svtkInformation* out) override;
  int IterativeRequestUpdateExtent(svtkInformation* in, svtkInformation* out) override;

  void ThreadedRequestData(svtkInformation* svtkNotUsed(request), svtkInformationVector** inputVector,
    svtkInformationVector* svtkNotUsed(outputVector), svtkImageData*** inDataVec,
    svtkImageData** outDataVec, int outExt[6], int threadId) override;

private:
  svtkImageRFFT(const svtkImageRFFT&) = delete;
  void operator=(const svtkImageRFFT&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageRFFT.h
