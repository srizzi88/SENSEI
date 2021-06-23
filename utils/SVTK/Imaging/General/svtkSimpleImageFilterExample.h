/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimpleImageFilterExample.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSimpleImageFilterExample
 * @brief   Simple example of an image-image filter.
 *
 * This is an example of a simple image-image filter. It copies it's input
 * to it's output (point by point). It shows how templates can be used
 * to support various data types.
 * @sa
 * svtkSimpleImageToImageFilter
 */

#ifndef svtkSimpleImageFilterExample_h
#define svtkSimpleImageFilterExample_h

#include "svtkImagingGeneralModule.h" // For export macro
#include "svtkSimpleImageToImageFilter.h"

class SVTKIMAGINGGENERAL_EXPORT svtkSimpleImageFilterExample : public svtkSimpleImageToImageFilter
{
public:
  static svtkSimpleImageFilterExample* New();
  svtkTypeMacro(svtkSimpleImageFilterExample, svtkSimpleImageToImageFilter);

protected:
  svtkSimpleImageFilterExample() {}
  ~svtkSimpleImageFilterExample() override {}

  void SimpleExecute(svtkImageData* input, svtkImageData* output) override;

private:
  svtkSimpleImageFilterExample(const svtkSimpleImageFilterExample&) = delete;
  void operator=(const svtkSimpleImageFilterExample&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkSimpleImageFilterExample.h
