/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDotProduct.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageDotProduct
 * @brief   Dot product of two vector images.
 *
 * svtkImageDotProduct interprets the scalar components of two images
 * as vectors and takes the dot product vector by vector (pixel by pixel).
 */

#ifndef svtkImageDotProduct_h
#define svtkImageDotProduct_h

#include "svtkImagingMathModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGMATH_EXPORT svtkImageDotProduct : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageDotProduct* New();
  svtkTypeMacro(svtkImageDotProduct, svtkThreadedImageAlgorithm);

  /**
   * Set the two inputs to this filter
   */
  virtual void SetInput1Data(svtkDataObject* in) { this->SetInputData(0, in); }
  virtual void SetInput2Data(svtkDataObject* in) { this->SetInputData(1, in); }

protected:
  svtkImageDotProduct();
  ~svtkImageDotProduct() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int threadId) override;

private:
  svtkImageDotProduct(const svtkImageDotProduct&) = delete;
  void operator=(const svtkImageDotProduct&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageDotProduct.h
