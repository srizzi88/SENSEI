/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageYIQToRGB.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageRGBToYIQ
 * @brief   Converts YIQ components to RGB.
 *
 * For each pixel with Y, I, and Q components this
 * filter output the color coded as RGB.
 * Output type must be the same as input type.
 */

#ifndef svtkImageYIQToRGB_h
#define svtkImageYIQToRGB_h

#include "svtkImagingColorModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCOLOR_EXPORT svtkImageYIQToRGB : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageYIQToRGB* New();
  svtkTypeMacro(svtkImageYIQToRGB, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkSetMacro(Maximum, double);
  svtkGetMacro(Maximum, double);

protected:
  svtkImageYIQToRGB();
  ~svtkImageYIQToRGB() override {}

  double Maximum; // Maximum value of pixel intensity allowed

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int ext[6], int id) override;

private:
  svtkImageYIQToRGB(const svtkImageYIQToRGB&) = delete;
  void operator=(const svtkImageYIQToRGB&) = delete;
};

#endif
