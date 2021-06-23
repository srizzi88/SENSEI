/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageContinuousErode3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageContinuousErode3D
 * @brief   Erosion implemented as a minimum.
 *
 * svtkImageContinuousErode3D replaces a pixel with the minimum over
 * an ellipsoidal neighborhood.  If KernelSize of an axis is 1, no processing
 * is done on that axis.
 */

#ifndef svtkImageContinuousErode3D_h
#define svtkImageContinuousErode3D_h

#include "svtkImageSpatialAlgorithm.h"
#include "svtkImagingMorphologicalModule.h" // For export macro

class svtkImageEllipsoidSource;

class SVTKIMAGINGMORPHOLOGICAL_EXPORT svtkImageContinuousErode3D : public svtkImageSpatialAlgorithm
{
public:
  //@{
  /**
   * Construct an instance of svtkImageContinuousErode3D filter.
   * By default zero values are eroded.
   */
  static svtkImageContinuousErode3D* New();
  svtkTypeMacro(svtkImageContinuousErode3D, svtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * This method sets the size of the neighborhood.  It also sets the
   * default middle of the neighborhood and computes the elliptical foot print.
   */
  void SetKernelSize(int size0, int size1, int size2);

protected:
  svtkImageContinuousErode3D();
  ~svtkImageContinuousErode3D() override;

  svtkImageEllipsoidSource* Ellipse;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkImageContinuousErode3D(const svtkImageContinuousErode3D&) = delete;
  void operator=(const svtkImageContinuousErode3D&) = delete;
};

#endif
