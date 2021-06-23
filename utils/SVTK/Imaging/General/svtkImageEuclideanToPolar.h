/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageEuclideanToPolar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageEuclideanToPolar
 * @brief   Converts 2D Euclidean coordinates to polar.
 *
 * For each pixel with vector components x,y, this filter outputs
 * theta in component0, and radius in component1.
 */

#ifndef svtkImageEuclideanToPolar_h
#define svtkImageEuclideanToPolar_h

#include "svtkImagingGeneralModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGGENERAL_EXPORT svtkImageEuclideanToPolar : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageEuclideanToPolar* New();
  svtkTypeMacro(svtkImageEuclideanToPolar, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Theta is an angle. Maximum specifies when it maps back to 0.
   * ThetaMaximum defaults to 255 instead of 2PI, because unsigned char
   * is expected as input. The output type must be the same as input type.
   */
  svtkSetMacro(ThetaMaximum, double);
  svtkGetMacro(ThetaMaximum, double);
  //@}

protected:
  svtkImageEuclideanToPolar();
  ~svtkImageEuclideanToPolar() override {}

  double ThetaMaximum;

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int ext[6], int id) override;

private:
  svtkImageEuclideanToPolar(const svtkImageEuclideanToPolar&) = delete;
  void operator=(const svtkImageEuclideanToPolar&) = delete;
};

#endif
