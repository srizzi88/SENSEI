/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMedian3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMedian3D
 * @brief   Median Filter
 *
 * svtkImageMedian3D a Median filter that replaces each pixel with the
 * median value from a rectangular neighborhood around that pixel.
 * Neighborhoods can be no more than 3 dimensional.  Setting one
 * axis of the neighborhood kernelSize to 1 changes the filter
 * into a 2D median.
 */

#ifndef svtkImageMedian3D_h
#define svtkImageMedian3D_h

#include "svtkImageSpatialAlgorithm.h"
#include "svtkImagingGeneralModule.h" // For export macro

class SVTKIMAGINGGENERAL_EXPORT svtkImageMedian3D : public svtkImageSpatialAlgorithm
{
public:
  static svtkImageMedian3D* New();
  svtkTypeMacro(svtkImageMedian3D, svtkImageSpatialAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This method sets the size of the neighborhood.  It also sets the
   * default middle of the neighborhood
   */
  void SetKernelSize(int size0, int size1, int size2);

  //@{
  /**
   * Return the number of elements in the median mask
   */
  svtkGetMacro(NumberOfElements, int);
  //@}

protected:
  svtkImageMedian3D();
  ~svtkImageMedian3D() override;

  int NumberOfElements;

  void ThreadedRequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector, svtkImageData*** inData, svtkImageData** outData,
    int extent[6], int id) override;

private:
  svtkImageMedian3D(const svtkImageMedian3D&) = delete;
  void operator=(const svtkImageMedian3D&) = delete;
};

#endif
