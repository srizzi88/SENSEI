/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageRGBToHSI.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageRGBToHSI
 * @brief   Converts RGB components to HSI.
 *
 * For each pixel with red, blue, and green components this
 * filter output the color coded as hue, saturation and intensity.
 * Output type must be the same as input type.
 */

#ifndef svtkImageRGBToHSI_h
#define svtkImageRGBToHSI_h

#include "svtkImagingColorModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCOLOR_EXPORT svtkImageRGBToHSI : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageRGBToHSI* New();
  svtkTypeMacro(svtkImageRGBToHSI, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Hue is an angle. Maximum specifies when it maps back to 0.  HueMaximum
   * defaults to 255 instead of 2PI, because unsigned char is expected as
   * input.  Maximum also specifies the maximum of the Saturation.
   */
  svtkSetMacro(Maximum, double);
  svtkGetMacro(Maximum, double);
  //@}

protected:
  svtkImageRGBToHSI();
  ~svtkImageRGBToHSI() override {}

  double Maximum;

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int ext[6], int id) override;

private:
  svtkImageRGBToHSI(const svtkImageRGBToHSI&) = delete;
  void operator=(const svtkImageRGBToHSI&) = delete;
};

#endif
