/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageContinuousDilate3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageContinuousDilate3D
 * @brief   Dilate implemented as a maximum.
 *
 * svtkImageContinuousDilate3D replaces a pixel with the maximum over
 * an ellipsoidal neighborhood.  If KernelSize of an axis is 1, no processing
 * is done on that axis.
 */

#ifndef svtkImageContinuousDilate3D_h
#define svtkImageContinuousDilate3D_h

#include "svtkImageSpatialAlgorithm.h"
#include "svtkImagingMorphologicalModule.h" // For export macro

class svtkImageEllipsoidSource;

class SVTKIMAGINGMORPHOLOGICAL_EXPORT svtkImageContinuousDilate3D : public svtkImageSpatialAlgorithm
{
public:
  //@{
  /**
   * Construct an instance of svtkImageContinuousDilate3D filter.
   * By default zero values are dilated.
   */
  static svtkImageContinuousDilate3D* New();
  svtkTypeMacro(svtkImageContinuousDilate3D, svtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * This method sets the size of the neighborhood.  It also sets the
   * default middle of the neighborhood and computes the elliptical foot print.
   */
  void SetKernelSize(int size0, int size1, int size2);

protected:
  svtkImageContinuousDilate3D();
  ~svtkImageContinuousDilate3D() override;

  svtkImageEllipsoidSource* Ellipse;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkImageContinuousDilate3D(const svtkImageContinuousDilate3D&) = delete;
  void operator=(const svtkImageContinuousDilate3D&) = delete;
};

#endif
