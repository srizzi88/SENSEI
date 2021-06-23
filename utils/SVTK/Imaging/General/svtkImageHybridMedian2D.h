/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageHybridMedian2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageHybridMedian2D
 * @brief   Median filter that preserves lines and
 * corners.
 *
 * svtkImageHybridMedian2D is a median filter that preserves thin lines and
 * corners.  It operates on a 5x5 pixel neighborhood.  It computes two values
 * initially: the median of the + neighbors and the median of the x
 * neighbors.  It then computes the median of these two values plus the center
 * pixel.  This result of this second median is the output pixel value.
 */

#ifndef svtkImageHybridMedian2D_h
#define svtkImageHybridMedian2D_h

#include "svtkImageSpatialAlgorithm.h"
#include "svtkImagingGeneralModule.h" // For export macro

class SVTKIMAGINGGENERAL_EXPORT svtkImageHybridMedian2D : public svtkImageSpatialAlgorithm
{
public:
  static svtkImageHybridMedian2D* New();
  svtkTypeMacro(svtkImageHybridMedian2D, svtkImageSpatialAlgorithm);

protected:
  svtkImageHybridMedian2D();
  ~svtkImageHybridMedian2D() override {}

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int outExt[6], int id) override;

private:
  svtkImageHybridMedian2D(const svtkImageHybridMedian2D&) = delete;
  void operator=(const svtkImageHybridMedian2D&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkImageHybridMedian2D.h
