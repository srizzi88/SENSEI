/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageGradientMagnitude.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageGradientMagnitude
 * @brief   Computes magnitude of the gradient.
 *
 *
 * svtkImageGradientMagnitude computes the gradient magnitude of an image.
 * Setting the dimensionality determines whether the gradient is computed on
 * 2D images, or 3D volumes.  The default is two dimensional XY images.
 *
 * @sa
 * svtkImageGradient svtkImageMagnitude
 */

#ifndef svtkImageGradientMagnitude_h
#define svtkImageGradientMagnitude_h

#include "svtkImagingGeneralModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGGENERAL_EXPORT svtkImageGradientMagnitude : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageGradientMagnitude* New();
  svtkTypeMacro(svtkImageGradientMagnitude, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * If "HandleBoundariesOn" then boundary pixels are duplicated
   * So central differences can get values.
   */
  svtkSetMacro(HandleBoundaries, svtkTypeBool);
  svtkGetMacro(HandleBoundaries, svtkTypeBool);
  svtkBooleanMacro(HandleBoundaries, svtkTypeBool);
  //@}

  //@{
  /**
   * Determines how the input is interpreted (set of 2d slices ...)
   */
  svtkSetClampMacro(Dimensionality, int, 2, 3);
  svtkGetMacro(Dimensionality, int);
  //@}

protected:
  svtkImageGradientMagnitude();
  ~svtkImageGradientMagnitude() override {}

  svtkTypeBool HandleBoundaries;
  int Dimensionality;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int extent[6], int id) override;

private:
  svtkImageGradientMagnitude(const svtkImageGradientMagnitude&) = delete;
  void operator=(const svtkImageGradientMagnitude&) = delete;
};

#endif
