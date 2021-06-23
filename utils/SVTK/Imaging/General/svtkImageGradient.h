/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageGradient.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageGradient
 * @brief   Computes the gradient vector.
 *
 * svtkImageGradient computes the gradient vector of an image.  The
 * vector results are stored as scalar components. The Dimensionality
 * determines whether to perform a 2d or 3d gradient. The default is
 * two dimensional XY gradient.  OutputScalarType is always
 * double. Gradient is computed using central differences.
 */

#ifndef svtkImageGradient_h
#define svtkImageGradient_h

#include "svtkImagingGeneralModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGGENERAL_EXPORT svtkImageGradient : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageGradient* New();
  svtkTypeMacro(svtkImageGradient, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Determines how the input is interpreted (set of 2d slices ...)
   */
  svtkSetClampMacro(Dimensionality, int, 2, 3);
  svtkGetMacro(Dimensionality, int);
  //@}

  //@{
  /**
   * Get/Set whether to handle boundaries.  If enabled, boundary
   * pixels are treated as duplicated so that central differencing
   * works for the boundary pixels.  If disabled, the output whole
   * extent of the image is reduced by one pixel.
   */
  svtkSetMacro(HandleBoundaries, svtkTypeBool);
  svtkGetMacro(HandleBoundaries, svtkTypeBool);
  svtkBooleanMacro(HandleBoundaries, svtkTypeBool);
  //@}

protected:
  svtkImageGradient();
  ~svtkImageGradient() override {}

  svtkTypeBool HandleBoundaries;
  int Dimensionality;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*,
    svtkImageData*** inData, svtkImageData** outData, int outExt[6], int threadId) override;

private:
  svtkImageGradient(const svtkImageGradient&) = delete;
  void operator=(const svtkImageGradient&) = delete;
};

#endif
