/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageNormalize.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageNormalize
 * @brief   Normalizes that scalar components for each point.
 *
 * For each point, svtkImageNormalize normalizes the vector defined by the
 * scalar components.  If the magnitude of this vector is zero, the output
 * vector is zero also.
 */

#ifndef svtkImageNormalize_h
#define svtkImageNormalize_h

#include "svtkImagingGeneralModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGGENERAL_EXPORT svtkImageNormalize : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageNormalize* New();
  svtkTypeMacro(svtkImageNormalize, svtkThreadedImageAlgorithm);

protected:
  svtkImageNormalize();
  ~svtkImageNormalize() override {}

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int extent[6], int id) override;

private:
  svtkImageNormalize(const svtkImageNormalize&) = delete;
  void operator=(const svtkImageNormalize&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageNormalize.h
