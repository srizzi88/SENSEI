/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSinusoidSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageSinusoidSource
 * @brief   Create an image with sinusoidal pixel values.
 *
 * svtkImageSinusoidSource just produces images with pixel values determined
 * by a sinusoid.
 */

#ifndef svtkImageSinusoidSource_h
#define svtkImageSinusoidSource_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingSourcesModule.h" // For export macro

class SVTKIMAGINGSOURCES_EXPORT svtkImageSinusoidSource : public svtkImageAlgorithm
{
public:
  static svtkImageSinusoidSource* New();
  svtkTypeMacro(svtkImageSinusoidSource, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set/Get the extent of the whole output image.
   */
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax, int zMin, int zMax);

  //@{
  /**
   * Set/Get the direction vector which determines the sinusoidal
   * orientation. The magnitude is ignored.
   */
  void SetDirection(double, double, double);
  void SetDirection(double dir[3]);
  svtkGetVector3Macro(Direction, double);
  //@}

  //@{
  /**
   * Set/Get the period of the sinusoid in pixels.
   */
  svtkSetMacro(Period, double);
  svtkGetMacro(Period, double);
  //@}

  //@{
  /**
   * Set/Get the phase: 0->2Pi.  0 => Cosine, pi/2 => Sine.
   */
  svtkSetMacro(Phase, double);
  svtkGetMacro(Phase, double);
  //@}

  //@{
  /**
   * Set/Get the magnitude of the sinusoid.
   */
  svtkSetMacro(Amplitude, double);
  svtkGetMacro(Amplitude, double);
  //@}

protected:
  svtkImageSinusoidSource();
  ~svtkImageSinusoidSource() override {}

  int WholeExtent[6];
  double Direction[3];
  double Period;
  double Phase;
  double Amplitude;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ExecuteDataWithInformation(svtkDataObject* data, svtkInformation* outInfo) override;

private:
  svtkImageSinusoidSource(const svtkImageSinusoidSource&) = delete;
  void operator=(const svtkImageSinusoidSource&) = delete;
};

#endif
