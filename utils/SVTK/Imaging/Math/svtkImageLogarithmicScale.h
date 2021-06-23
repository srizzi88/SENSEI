/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageLogarithmicScale.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageLogarithmicScale
 * @brief   Passes each pixel through log function.
 *
 * svtkImageLogarithmicScale passes each pixel through the function
 * c*log(1+x).  It also handles negative values with the function
 * -c*log(1-x).
 */

#ifndef svtkImageLogarithmicScale_h
#define svtkImageLogarithmicScale_h

#include "svtkImagingMathModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGMATH_EXPORT svtkImageLogarithmicScale : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageLogarithmicScale* New();
  svtkTypeMacro(svtkImageLogarithmicScale, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the scale factor for the logarithmic function.
   */
  svtkSetMacro(Constant, double);
  svtkGetMacro(Constant, double);
  //@}

protected:
  svtkImageLogarithmicScale();
  ~svtkImageLogarithmicScale() override {}

  double Constant;

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int outExt[6], int id) override;

private:
  svtkImageLogarithmicScale(const svtkImageLogarithmicScale&) = delete;
  void operator=(const svtkImageLogarithmicScale&) = delete;
};

#endif
