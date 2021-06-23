/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLagrangianParticleTracker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBilinearQuadIntersection
 * @brief   Class to perform non planar quad intersection
 *
 * Class for non planar quad intersection.
 * This class is an updated and fixed version of the code by Ramsey et al.
 * (http://shaunramsey.com/research/bp/).
 */

#ifndef svtkBilinearQuadIntersection_h
#define svtkBilinearQuadIntersection_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkVector.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkBilinearQuadIntersection
{
public:
  svtkBilinearQuadIntersection(const svtkVector3d& pt00, const svtkVector3d& Pt01,
    const svtkVector3d& Pt10, const svtkVector3d& Pt11);
  svtkBilinearQuadIntersection() = default;

  //@{
  /**
   * Get direct access to the underlying point data
   */
  double* GetP00Data();
  double* GetP01Data();
  double* GetP10Data();
  double* GetP11Data();
  //}@

  /**
   * Compute cartesian coordinates of point in the quad
   * using parameteric coordinates
   */
  svtkVector3d ComputeCartesianCoordinates(double u, double v);

  /**
   * Compute the intersection between a ray r->d and the quad
   */
  bool RayIntersection(const svtkVector3d& r, const svtkVector3d& d, svtkVector3d& uv);

private:
  svtkVector3d Point00;
  svtkVector3d Point01;
  svtkVector3d Point10;
  svtkVector3d Point11;
  int AxesSwapping = 0;
};
#endif // svtkBilinearQuadIntersection_h
// SVTK-HeaderTest-Exclude: svtkBilinearQuadIntersection.h
