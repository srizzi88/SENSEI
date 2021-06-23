/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricHenneberg.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricHenneberg
 * @brief   Generate Henneberg's minimal surface.
 *
 * svtkParametricHenneberg generates Henneberg's minimal surface parametrically.
 * Henneberg's minimal surface is discussed further at
 * <a href="http://mathworld.wolfram.com/HennebergsMinimalSurface.html">Math World</a>.
 * @par Thanks:
 * Tim Meehan
 */

#ifndef svtkParametricHenneberg_h
#define svtkParametricHenneberg_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkParametricFunction.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkParametricHenneberg : public svtkParametricFunction
{
public:
  svtkTypeMacro(svtkParametricHenneberg, svtkParametricFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct Henneberg's minimal surface with the following parameters:
   * (MinimumU, MaximumU) = (-1., 1.),
   * (MinimumV, MaximumV) = (-pi/.2, pi/2.),
   * JoinU = 0, JoinV = 0,
   * TwistU = 0, TwistV = 0;
   * ClockwiseOrdering = 0,
   * DerivativesAvailable = 1,
   */
  static svtkParametricHenneberg* New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  /**
   * Henneberg's minimal surface.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), D_u\vec{f} = (dx/du, dy/du, dz/du), D_v\vec{f} = (dx/dv, dy/dv, dz/dv)\f$ .
   * Then the normal is \f$N = D_u\vec{f} \times D_v\vec{f}\f$ .
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) override;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.
   * This method simply returns 0.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) override;

protected:
  svtkParametricHenneberg();
  ~svtkParametricHenneberg() override;

private:
  svtkParametricHenneberg(const svtkParametricHenneberg&) = delete;
  void operator=(const svtkParametricHenneberg&) = delete;
};

#endif
