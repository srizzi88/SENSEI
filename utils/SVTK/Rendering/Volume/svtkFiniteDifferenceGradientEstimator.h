/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFiniteDifferenceGradientEstimator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkFiniteDifferenceGradientEstimator
 * @brief   Use finite differences to estimate gradient.
 *
 *
 *  svtkFiniteDifferenceGradientEstimator is a concrete subclass of
 *  svtkEncodedGradientEstimator that uses a central differences technique to
 *  estimate the gradient. The gradient at some sample location (x,y,z)
 *  would be estimated by:
 *
 *       nx = (f(x-dx,y,z) - f(x+dx,y,z)) / 2*dx;
 *       ny = (f(x,y-dy,z) - f(x,y+dy,z)) / 2*dy;
 *       nz = (f(x,y,z-dz) - f(x,y,z+dz)) / 2*dz;
 *
 *  This value is normalized to determine a unit direction vector and a
 *  magnitude. The normal is computed in voxel space, and
 *  dx = dy = dz = SampleSpacingInVoxels. A scaling factor is applied to
 *  convert this normal from voxel space to world coordinates.
 *
 * @sa
 * svtkEncodedGradientEstimator
 */

#ifndef svtkFiniteDifferenceGradientEstimator_h
#define svtkFiniteDifferenceGradientEstimator_h

#include "svtkEncodedGradientEstimator.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class SVTKRENDERINGVOLUME_EXPORT svtkFiniteDifferenceGradientEstimator
  : public svtkEncodedGradientEstimator
{
public:
  svtkTypeMacro(svtkFiniteDifferenceGradientEstimator, svtkEncodedGradientEstimator);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a svtkFiniteDifferenceGradientEstimator with
   * a SampleSpacingInVoxels of 1.
   */
  static svtkFiniteDifferenceGradientEstimator* New();

  //@{
  /**
   * Set/Get the spacing between samples for the finite differences
   * method used to compute the normal. This spacing is in voxel units.
   */
  svtkSetMacro(SampleSpacingInVoxels, int);
  svtkGetMacro(SampleSpacingInVoxels, int);
  //@}

  // The sample spacing between samples taken for the normal estimation
  int SampleSpacingInVoxels;

protected:
  svtkFiniteDifferenceGradientEstimator();
  ~svtkFiniteDifferenceGradientEstimator() override;

  /**
   * Recompute the encoded normals and gradient magnitudes.
   */
  void UpdateNormals(void) override;

private:
  svtkFiniteDifferenceGradientEstimator(const svtkFiniteDifferenceGradientEstimator&) = delete;
  void operator=(const svtkFiniteDifferenceGradientEstimator&) = delete;
};

#endif
