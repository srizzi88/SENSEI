/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLagrangianParticle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBilinearQuadIntersection.h"

#include "svtkMathUtilities.h"

#include <iostream>

int TestBilinearQuadIntersection(int, char*[])
{
  // --------- Static methods ------------
  svtkVector3d p00(0, 0, 0);
  svtkVector3d p01(0, 1, 0);
  svtkVector3d p10(1, 0, 0);
  svtkVector3d p11(1, 1, 0.5);
  svtkBilinearQuadIntersection quad(p00, p01, p10, p11);

  // ---- ComputeCartesianCoordinates ----
  double u = 0.3;
  double v = 0.7;
  svtkVector3d coord = quad.ComputeCartesianCoordinates(u, v);
  if (coord.GetX() != u || coord.GetY() != v || coord.GetZ() != 0.105)
  {
    std::cerr << "svtkBilinearQuadIntersection::ComputeCartesianCoordinates got unexpected results :"
              << std::endl;
    std::cerr << coord.GetX() << " " << coord.GetY() << " " << coord.GetZ() << std::endl;
    return EXIT_FAILURE;
  }

  // ---- RayIntersection ----
  svtkVector3d r(0.5, 0.5, -1);
  svtkVector3d q(0, 0, 1);
  svtkVector3d uv;
  quad.RayIntersection(r, q, uv);
  if (uv.GetX() != 0.5 || uv.GetY() != 0.5 || uv.GetZ() != 1.125)
  {
    std::cerr << "svtkBilinearQuadIntersection::RayIntersection got unexpected results :"
              << std::endl;
    std::cerr << (uv.GetX() != 0.5) << " " << (uv.GetY() != 0.5) << " " << (uv.GetZ() != 1.125)
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
