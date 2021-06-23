/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSobel3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageSobel3D
 * @brief   Computes a vector field using sobel functions.
 *
 * svtkImageSobel3D computes a vector field from a scalar field by using
 * Sobel functions.  The number of vector components is 3 because
 * the input is a volume.  Output is always doubles.  A little creative
 * liberty was used to extend the 2D sobel kernels into 3D.
 */

#ifndef svtkImageSobel3D_h
#define svtkImageSobel3D_h

#include "svtkImageSpatialAlgorithm.h"
#include "svtkImagingGeneralModule.h" // For export macro

class SVTKIMAGINGGENERAL_EXPORT svtkImageSobel3D : public svtkImageSpatialAlgorithm
{
public:
  static svtkImageSobel3D* New();
  svtkTypeMacro(svtkImageSobel3D, svtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkImageSobel3D();
  ~svtkImageSobel3D() override {}

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int outExt[6], int id) override;
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkImageSobel3D(const svtkImageSobel3D&) = delete;
  void operator=(const svtkImageSobel3D&) = delete;
};

#endif
