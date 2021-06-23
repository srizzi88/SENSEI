/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageCityBlockDistance.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageCityBlockDistance
 * @brief   1,2 or 3D distance map.
 *
 * svtkImageCityBlockDistance creates a distance map using the city block
 * (Manhatten) distance measure.  The input is a mask.  Zero values are
 * considered boundaries.  The output pixel is the minimum of the input pixel
 * and the distance to a boundary (or neighbor value + 1 unit).
 * distance values are calculated in pixels.
 * The filter works by taking 6 passes (for 3d distance map): 2 along each
 * axis (forward and backward). Each pass keeps a running minimum distance.
 * For some reason, I preserve the sign if the distance.  If the input
 * mask is initially negative, the output distances will be negative.
 * Distances maps can have inside (negative regions)
 * and outsides (positive regions).
 */

#ifndef svtkImageCityBlockDistance_h
#define svtkImageCityBlockDistance_h

#include "svtkImageDecomposeFilter.h"
#include "svtkImagingGeneralModule.h" // For export macro

class SVTKIMAGINGGENERAL_EXPORT svtkImageCityBlockDistance : public svtkImageDecomposeFilter
{
public:
  static svtkImageCityBlockDistance* New();
  svtkTypeMacro(svtkImageCityBlockDistance, svtkImageDecomposeFilter);

protected:
  svtkImageCityBlockDistance();
  ~svtkImageCityBlockDistance() override {}

  int IterativeRequestUpdateExtent(svtkInformation* in, svtkInformation* out) override;
  int IterativeRequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void AllocateOutputScalars(
    svtkImageData* outData, int* updateExtent, int* wholeExtent, svtkInformation* outInfo);

private:
  svtkImageCityBlockDistance(const svtkImageCityBlockDistance&) = delete;
  void operator=(const svtkImageCityBlockDistance&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageCityBlockDistance.h
