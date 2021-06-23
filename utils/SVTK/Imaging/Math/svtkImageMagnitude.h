/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageMagnitude.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageMagnitude
 * @brief   Colapses components with magnitude function..
 *
 * svtkImageMagnitude takes the magnitude of the components.
 */

#ifndef svtkImageMagnitude_h
#define svtkImageMagnitude_h

#include "svtkImagingMathModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGMATH_EXPORT svtkImageMagnitude : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageMagnitude* New();
  svtkTypeMacro(svtkImageMagnitude, svtkThreadedImageAlgorithm);

protected:
  svtkImageMagnitude();
  ~svtkImageMagnitude() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int outExt[6], int id) override;

private:
  svtkImageMagnitude(const svtkImageMagnitude&) = delete;
  void operator=(const svtkImageMagnitude&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageMagnitude.h
