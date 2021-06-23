/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageRGBToYIQ.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageRGBToYIQ
 * @brief   Converts RGB components to YIQ.
 *
 * For each pixel with red, blue, and green components this
 * filter output the color coded as YIQ.
 * Output type must be the same as input type.
 */

#ifndef svtkImageRGBToYIQ_h
#define svtkImageRGBToYIQ_h

#include "svtkImagingColorModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCOLOR_EXPORT svtkImageRGBToYIQ : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageRGBToYIQ* New();
  svtkTypeMacro(svtkImageRGBToYIQ, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkSetMacro(Maximum, double);
  svtkGetMacro(Maximum, double);

protected:
  svtkImageRGBToYIQ();
  ~svtkImageRGBToYIQ() override {}

  double Maximum; // Maximum value of pixel intensity allowed

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int ext[6], int id) override;

private:
  svtkImageRGBToYIQ(const svtkImageRGBToYIQ&) = delete;
  void operator=(const svtkImageRGBToYIQ&) = delete;
};

#endif
