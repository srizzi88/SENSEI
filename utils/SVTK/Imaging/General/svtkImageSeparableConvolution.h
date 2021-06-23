/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSeparableConvolution.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageSeparableConvolution
 * @brief    3 1D convolutions on an image
 *
 * svtkImageSeparableConvolution performs a convolution along the X, Y,
 * and Z axes of an image, based on the three different 1D convolution
 * kernels.  The kernels must be of odd size, and are considered to be
 * centered at (int)((kernelsize - 1) / 2.0 ).  If a kernel is nullptr,
 * that dimension is skipped.  This filter is designed to efficiently
 * convolve separable filters that can be decomposed into 1 or more 1D
 * convolutions.  It also handles arbitrarily large kernel sizes, and
 * uses edge replication to handle boundaries.
 */

#ifndef svtkImageSeparableConvolution_h
#define svtkImageSeparableConvolution_h

#include "svtkImageDecomposeFilter.h"
#include "svtkImagingGeneralModule.h" // For export macro

class svtkFloatArray;

class SVTKIMAGINGGENERAL_EXPORT svtkImageSeparableConvolution : public svtkImageDecomposeFilter
{
public:
  static svtkImageSeparableConvolution* New();
  svtkTypeMacro(svtkImageSeparableConvolution, svtkImageDecomposeFilter);

  // Set the X convolution kernel, a null value indicates no convolution to
  // be done.  The kernel must be of odd length
  virtual void SetXKernel(svtkFloatArray*);
  svtkGetObjectMacro(XKernel, svtkFloatArray);

  // Set the Y convolution kernel, a null value indicates no convolution to
  // be done The kernel must be of odd length
  virtual void SetYKernel(svtkFloatArray*);
  svtkGetObjectMacro(YKernel, svtkFloatArray);

  // Set the Z convolution kernel, a null value indicates no convolution to
  // be done The kernel must be of odd length
  virtual void SetZKernel(svtkFloatArray*);
  svtkGetObjectMacro(ZKernel, svtkFloatArray);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Overload standard modified time function. If kernel arrays are modified,
   * then this object is modified as well.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkImageSeparableConvolution();
  ~svtkImageSeparableConvolution() override;

  svtkFloatArray* XKernel;
  svtkFloatArray* YKernel;
  svtkFloatArray* ZKernel;

  int IterativeRequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int IterativeRequestInformation(svtkInformation* in, svtkInformation* out) override;
  int IterativeRequestUpdateExtent(svtkInformation* in, svtkInformation* out) override;

private:
  svtkImageSeparableConvolution(const svtkImageSeparableConvolution&) = delete;
  void operator=(const svtkImageSeparableConvolution&) = delete;
};

#endif
