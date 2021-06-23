/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageVariance3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageVariance3D
 * @brief   Variance in a neighborhood.
 *
 * svtkImageVariance3D replaces each pixel with a measurement of
 * pixel variance in a elliptical neighborhood centered on that pixel.
 * The value computed is not exactly the variance.
 * The difference between the neighbor values and center value is computed
 * and squared for each neighbor.  These values are summed and divided by
 * the total number of neighbors to produce the output value.
 */

#ifndef svtkImageVariance3D_h
#define svtkImageVariance3D_h

#include "svtkImageSpatialAlgorithm.h"
#include "svtkImagingGeneralModule.h" // For export macro

class svtkImageEllipsoidSource;

class SVTKIMAGINGGENERAL_EXPORT svtkImageVariance3D : public svtkImageSpatialAlgorithm
{
public:
  static svtkImageVariance3D* New();
  svtkTypeMacro(svtkImageVariance3D, svtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This method sets the size of the neighborhood.  It also sets the default
   * middle of the neighborhood and computes the Elliptical foot print.
   */
  void SetKernelSize(int size0, int size1, int size2);

protected:
  svtkImageVariance3D();
  ~svtkImageVariance3D() override;

  svtkImageEllipsoidSource* Ellipse;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkImageVariance3D(const svtkImageVariance3D&) = delete;
  void operator=(const svtkImageVariance3D&) = delete;
};

#endif
