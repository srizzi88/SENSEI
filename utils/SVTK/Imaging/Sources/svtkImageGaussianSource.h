/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageGaussianSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageGaussianSource
 * @brief   Create an image with Gaussian pixel values.
 *
 * svtkImageGaussianSource just produces images with pixel values determined
 * by a Gaussian.
 */

#ifndef svtkImageGaussianSource_h
#define svtkImageGaussianSource_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingSourcesModule.h" // For export macro

class SVTKIMAGINGSOURCES_EXPORT svtkImageGaussianSource : public svtkImageAlgorithm
{
public:
  static svtkImageGaussianSource* New();
  svtkTypeMacro(svtkImageGaussianSource, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set/Get the extent of the whole output image.
   */
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax, int zMin, int zMax);

  //@{
  /**
   * Set/Get the center of the Gaussian.
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVector3Macro(Center, double);
  //@}

  //@{
  /**
   * Set/Get the Maximum value of the gaussian
   */
  svtkSetMacro(Maximum, double);
  svtkGetMacro(Maximum, double);
  //@}

  //@{
  /**
   * Set/Get the standard deviation of the gaussian
   */
  svtkSetMacro(StandardDeviation, double);
  svtkGetMacro(StandardDeviation, double);
  //@}

protected:
  svtkImageGaussianSource();
  ~svtkImageGaussianSource() override {}

  double StandardDeviation;
  int WholeExtent[6];
  double Center[3];
  double Maximum;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkImageGaussianSource(const svtkImageGaussianSource&) = delete;
  void operator=(const svtkImageGaussianSource&) = delete;
};

#endif
