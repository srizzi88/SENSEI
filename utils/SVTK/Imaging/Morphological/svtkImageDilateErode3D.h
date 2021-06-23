/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDilateErode3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageDilateErode3D
 * @brief   Dilates one value and erodes another.
 *
 * svtkImageDilateErode3D will dilate one value and erode another.
 * It uses an elliptical foot print, and only erodes/dilates on the
 * boundary of the two values.  The filter is restricted to the
 * X, Y, and Z axes for now.  It can degenerate to a 2 or 1 dimensional
 * filter by setting the kernel size to 1 for a specific axis.
 */

#ifndef svtkImageDilateErode3D_h
#define svtkImageDilateErode3D_h

#include "svtkImageSpatialAlgorithm.h"
#include "svtkImagingMorphologicalModule.h" // For export macro

class svtkImageEllipsoidSource;

class SVTKIMAGINGMORPHOLOGICAL_EXPORT svtkImageDilateErode3D : public svtkImageSpatialAlgorithm
{
public:
  //@{
  /**
   * Construct an instance of svtkImageDilateErode3D filter.
   * By default zero values are dilated.
   */
  static svtkImageDilateErode3D* New();
  svtkTypeMacro(svtkImageDilateErode3D, svtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * This method sets the size of the neighborhood.  It also sets the
   * default middle of the neighborhood and computes the elliptical foot print.
   */
  void SetKernelSize(int size0, int size1, int size2);

  //@{
  /**
   * Set/Get the Dilate and Erode values to be used by this filter.
   */
  svtkSetMacro(DilateValue, double);
  svtkGetMacro(DilateValue, double);
  svtkSetMacro(ErodeValue, double);
  svtkGetMacro(ErodeValue, double);
  //@}

protected:
  svtkImageDilateErode3D();
  ~svtkImageDilateErode3D() override;

  svtkImageEllipsoidSource* Ellipse;
  double DilateValue;
  double ErodeValue;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

private:
  svtkImageDilateErode3D(const svtkImageDilateErode3D&) = delete;
  void operator=(const svtkImageDilateErode3D&) = delete;
};

#endif
