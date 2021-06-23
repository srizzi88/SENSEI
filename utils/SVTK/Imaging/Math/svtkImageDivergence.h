/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageDivergence.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageDivergence
 * @brief   Divergence of a vector field.
 *
 * svtkImageDivergence takes a 3D vector field
 * and creates a scalar field which
 * which represents the rate of change of the vector field.
 * The definition of Divergence:
 * Given V = P(x,y,z), Q(x,y,z), R(x,y,z),
 * Divergence = dP/dx + dQ/dy + dR/dz.
 */

#ifndef svtkImageDivergence_h
#define svtkImageDivergence_h

#include "svtkImagingMathModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGMATH_EXPORT svtkImageDivergence : public svtkThreadedImageAlgorithm
{
public:
  static svtkImageDivergence* New();
  svtkTypeMacro(svtkImageDivergence, svtkThreadedImageAlgorithm);

protected:
  svtkImageDivergence();
  ~svtkImageDivergence() override {}

  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void ThreadedExecute(svtkImageData* inData, svtkImageData* outData, int ext[6], int id) override;

private:
  svtkImageDivergence(const svtkImageDivergence&) = delete;
  void operator=(const svtkImageDivergence&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkImageDivergence.h
