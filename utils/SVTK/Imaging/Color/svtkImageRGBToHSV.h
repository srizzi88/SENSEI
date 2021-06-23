/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageRGBToHSV.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageRGBToHSV
 * @brief   Converts RGB components to HSV.
 *
 * For each pixel with red, blue, and green components this
 * filter output the color coded as hue, saturation and value.
 * Output type must be the same as input type.
 */

#ifndef svtkImageRGBToHSV_h
#define svtkImageRGBToHSV_h

#include "svtkImagingColorModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCOLOR_EXPORT svtkImageRGBToHSV : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageRGBToHSV* New();
  svtkTypeMacro(svtkImageRGBToHSV, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Hue is an angle. Maximum specifies when it maps back to 0.  HueMaximum
  // defaults to 255 instead of 2PI, because unsigned char is expected as
  // input.  Maximum also specifies the maximum of the Saturation.
  svtkSetMacro(Maximum, double);
  svtkGetMacro(Maximum, double);

protected:
  svtkImageRGBToHSV();
  ~svtkImageRGBToHSV() override {}

  double Maximum;

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int ext[6], int id) override;

private:
  svtkImageRGBToHSV(const svtkImageRGBToHSV&) = delete;
  void operator=(const svtkImageRGBToHSV&) = delete;
};

#endif
