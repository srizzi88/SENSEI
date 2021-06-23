/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMapToRGBA.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMapToRGBA
 * @brief   map the input image through a lookup table
 *
 * This filter has been replaced by svtkImageMapToColors, which provided
 * additional features.  Use svtkImageMapToColors instead.
 *
 * @sa
 * svtkLookupTable
 */

#ifndef svtkImageMapToRGBA_h
#define svtkImageMapToRGBA_h

#include "svtkImageMapToColors.h"
#include "svtkImagingColorModule.h" // For export macro

class SVTKIMAGINGCOLOR_EXPORT svtkImageMapToRGBA : public svtkImageMapToColors
{
public:
  static svtkImageMapToRGBA* New();
  svtkTypeMacro(svtkImageMapToRGBA, svtkImageMapToColors);

protected:
  svtkImageMapToRGBA() {}
  ~svtkImageMapToRGBA() override {}

private:
  svtkImageMapToRGBA(const svtkImageMapToRGBA&) = delete;
  void operator=(const svtkImageMapToRGBA&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageMapToRGBA.h
