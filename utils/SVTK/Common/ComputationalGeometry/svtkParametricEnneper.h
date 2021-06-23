/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricEnneper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricEnneper
 * @brief   Generate Enneper's surface.
 *
 * svtkParametricEnneper generates Enneper's surface.
 * Enneper's surface is a a self-intersecting minimal surface
 * possessing constant negative Gaussian curvature
 *
 * For further information about this surface, please consult the
 * technical description "Parametric surfaces" in http://www.svtk.org/publications
 * in the "SVTK Technical Documents" section in the VTk.org web pages.
 *
 * @par Thanks:
 * Andrew Maclean andrew.amaclean@gmail.com for creating and contributing the
 * class.
 *
 */

#ifndef svtkParametricEnneper_h
#define svtkParametricEnneper_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkParametricFunction.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkParametricEnneper : public svtkParametricFunction
{
public:
  svtkTypeMacro(svtkParametricEnneper, svtkParametricFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct Enneper's surface with the following parameters:
   * MinimumU = -2, MaximumU = 2,
   * MinimumV = -2, MaximumV = 2,
   * JoinU = 0, JoinV = 0,
   * TwistU = 0, TwistV = 0,
   * ClockwiseOrdering = 0,
   * DerivativesAvailable = 1
   */
  static svtkParametricEnneper* New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  /**
   * Enneper's surface.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$ .
   * Then the normal is \f$N = Du X Dv\f$ .
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) override;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.

   * uv are the parameters with Pt being the cartesian point,
   * Duvw are the derivatives of this point with respect to u, v and w.
   * Pt, Duvw are obtained from Evaluate().

   * This function is only called if the ScalarMode has the value
   * svtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED

   * If the user does not need to calculate a scalar, then the
   * instantiated function should return zero.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) override;

protected:
  svtkParametricEnneper();
  ~svtkParametricEnneper() override;

private:
  svtkParametricEnneper(const svtkParametricEnneper&) = delete;
  void operator=(const svtkParametricEnneper&) = delete;
};

#endif
