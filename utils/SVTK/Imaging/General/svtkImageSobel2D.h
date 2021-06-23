/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSobel2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageSobel2D
 * @brief   Computes a vector field using sobel functions.
 *
 * svtkImageSobel2D computes a vector field from a scalar field by using
 * Sobel functions.  The number of vector components is 2 because
 * the input is an image.  Output is always doubles.
 */

#ifndef svtkImageSobel2D_h
#define svtkImageSobel2D_h

#include "svtkImageSpatialAlgorithm.h"
#include "svtkImagingGeneralModule.h" // For export macro

class SVTKIMAGINGGENERAL_EXPORT svtkImageSobel2D : public svtkImageSpatialAlgorithm
{
public:
  static svtkImageSobel2D* New();
  svtkTypeMacro(svtkImageSobel2D, svtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkImageSobel2D();
  ~svtkImageSobel2D() override {}

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int outExt[6], int id) override;
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkImageSobel2D(const svtkImageSobel2D&) = delete;
  void operator=(const svtkImageSobel2D&) = delete;
};

#endif
