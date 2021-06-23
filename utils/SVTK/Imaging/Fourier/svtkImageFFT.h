/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageFFT.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageFFT
 * @brief    Fast Fourier Transform.
 *
 * svtkImageFFT implements a fast Fourier transform.  The input
 * can have real or complex data in any components and data types, but
 * the output is always complex doubles with real values in component0, and
 * imaginary values in component1.  The filter is fastest for images that
 * have power of two sizes.  The filter uses a butterfly diagram for each
 * prime factor of the dimension.  This makes images with prime number dimensions
 * (i.e. 17x17) much slower to compute.  Multi dimensional (i.e volumes)
 * FFT's are decomposed so that each axis executes serially.
 */

#ifndef svtkImageFFT_h
#define svtkImageFFT_h

#include "svtkImageFourierFilter.h"
#include "svtkImagingFourierModule.h" // For export macro

class SVTKIMAGINGFOURIER_EXPORT svtkImageFFT : public svtkImageFourierFilter
{
public:
  static svtkImageFFT* New();
  svtkTypeMacro(svtkImageFFT, svtkImageFourierFilter);

protected:
  svtkImageFFT() {}
  ~svtkImageFFT() override {}

  int IterativeRequestInformation(svtkInformation* in, svtkInformation* out) override;
  int IterativeRequestUpdateExtent(svtkInformation* in, svtkInformation* out) override;

  void ThreadedRequestData(svtkInformation* svtkNotUsed(request), svtkInformationVector** inputVector,
    svtkInformationVector* svtkNotUsed(outputVector), svtkImageData*** inDataVec,
    svtkImageData** outDataVec, int outExt[6], int threadId) override;

private:
  svtkImageFFT(const svtkImageFFT&) = delete;
  void operator=(const svtkImageFFT&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageFFT.h
