/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageHSIToRGB.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageHSIToRGB
 * @brief   Converts HSI components to RGB.
 *
 * For each pixel with hue, saturation and intensity components this filter
 * outputs the color coded as red, green, blue.  Output type must be the same
 * as input type.
 *
 * @sa
 * svtkImageRGBToHSI
 */

#ifndef svtkImageHSIToRGB_h
#define svtkImageHSIToRGB_h

#include "svtkImagingColorModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCOLOR_EXPORT svtkImageHSIToRGB : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageHSIToRGB* New();
  svtkTypeMacro(svtkImageHSIToRGB, svtkThreadedImageAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Hue is an angle. Maximum specifies when it maps back to 0.
   * HueMaximum defaults to 255 instead of 2PI, because unsigned char
   * is expected as input.
   * Maximum also specifies the maximum of the Saturation, and R, G, B.
   */
  svtkSetMacro(Maximum, double);
  svtkGetMacro(Maximum, double);
  //@}

protected:
  svtkImageHSIToRGB();
  ~svtkImageHSIToRGB() override {}

  double Maximum;

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int ext[6], int id) override;

private:
  svtkImageHSIToRGB(const svtkImageHSIToRGB&) = delete;
  void operator=(const svtkImageHSIToRGB&) = delete;
};

#endif
