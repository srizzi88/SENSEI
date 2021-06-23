/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageLuminance.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageLuminance
 * @brief   Computes the luminance of the input
 *
 * svtkImageLuminance calculates luminance from an rgb input.
 */

#ifndef svtkImageLuminance_h
#define svtkImageLuminance_h

#include "svtkImagingColorModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCOLOR_EXPORT svtkImageLuminance : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageLuminance* New();
  svtkTypeMacro(svtkImageLuminance, svtkThreadedImageAlgorithm);

protected:
  svtkImageLuminance();
  ~svtkImageLuminance() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int outExt[6], int id) override;

private:
  svtkImageLuminance(const svtkImageLuminance&) = delete;
  void operator=(const svtkImageLuminance&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageLuminance.h
