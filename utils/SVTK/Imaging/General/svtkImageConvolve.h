/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageConvolve.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageConvolve
 * @brief   Convolution of an image with a kernel.
 *
 * svtkImageConvolve convolves the image with a 3D NxNxN kernel or a
 * 2D NxN kernel.  The output image is cropped to the same size as
 * the input.
 */

#ifndef svtkImageConvolve_h
#define svtkImageConvolve_h

#include "svtkImagingGeneralModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGGENERAL_EXPORT svtkImageConvolve : public svtkThreadedImageAlgorithm
{
public:
  //@{
  /**
   * Construct an instance of svtkImageConvolve filter.
   */
  static svtkImageConvolve* New();
  svtkTypeMacro(svtkImageConvolve, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Get the kernel size
   */
  svtkGetVector3Macro(KernelSize, int);
  //@}

  //@{
  /**
   * Set the kernel to be a given 3x3 or 5x5 or 7x7 kernel.
   */
  void SetKernel3x3(const double kernel[9]);
  void SetKernel5x5(const double kernel[25]);
  //@}

  void SetKernel7x7(const double kernel[49]);

  //@{
  /**
   * Return an array that contains the kernel.
   */
  double* GetKernel3x3() SVTK_SIZEHINT(9);
  void GetKernel3x3(double kernel[9]);
  double* GetKernel5x5() SVTK_SIZEHINT(25);
  void GetKernel5x5(double kernel[25]);
  //@}

  double* GetKernel7x7() SVTK_SIZEHINT(49);
  void GetKernel7x7(double kernel[49]);

  /**
   * Set the kernel to be a 3x3x3 or 5x5x5 or 7x7x7 kernel.
   */
  void SetKernel3x3x3(const double kernel[27]);

  void SetKernel5x5x5(const double kernel[125]);
  void SetKernel7x7x7(const double kernel[343]);

  //@{
  /**
   * Return an array that contains the kernel
   */
  double* GetKernel3x3x3() SVTK_SIZEHINT(27);
  void GetKernel3x3x3(double kernel[27]);
  //@}

  double* GetKernel5x5x5() SVTK_SIZEHINT(125);
  void GetKernel5x5x5(double kernel[125]);
  double* GetKernel7x7x7() SVTK_SIZEHINT(343);
  void GetKernel7x7x7(double kernel[343]);

protected:
  svtkImageConvolve();
  ~svtkImageConvolve() override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int outExt[6], int id) override;

  void GetKernel(double* kernel);
  double* GetKernel();
  void SetKernel(const double* kernel, int sizeX, int sizeY, int sizeZ);

  int KernelSize[3];
  double Kernel[343];

private:
  svtkImageConvolve(const svtkImageConvolve&) = delete;
  void operator=(const svtkImageConvolve&) = delete;
};

#endif
