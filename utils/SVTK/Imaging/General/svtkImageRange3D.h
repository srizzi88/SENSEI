/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageRange3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageRange3D
 * @brief   Max - min of a circular neighborhood.
 *
 * svtkImageRange3D replaces a pixel with the maximum minus minimum over
 * an ellipsoidal neighborhood.  If KernelSize of an axis is 1, no processing
 * is done on that axis.
 */

#ifndef svtkImageRange3D_h
#define svtkImageRange3D_h

#include "svtkImageSpatialAlgorithm.h"
#include "svtkImagingGeneralModule.h" // For export macro

class svtkImageEllipsoidSource;

class SVTKIMAGINGGENERAL_EXPORT svtkImageRange3D : public svtkImageSpatialAlgorithm
{
public:
  static svtkImageRange3D* New();
  svtkTypeMacro(svtkImageRange3D, svtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This method sets the size of the neighborhood.  It also sets the
   * default middle of the neighborhood and computes the elliptical foot print.
   */
  void SetKernelSize(int size0, int size1, int size2);

protected:
  svtkImageRange3D();
  ~svtkImageRange3D() override;

  svtkImageEllipsoidSource* Ellipse;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkImageRange3D(const svtkImageRange3D&) = delete;
  void operator=(const svtkImageRange3D&) = delete;
};

#endif
