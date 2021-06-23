/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricMobius.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricMobius
 * @brief   Generate a Mobius strip.
 *
 * svtkParametricMobius generates a Mobius strip.
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

#ifndef svtkParametricMobius_h
#define svtkParametricMobius_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkParametricFunction.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkParametricMobius : public svtkParametricFunction
{
public:
  svtkTypeMacro(svtkParametricMobius, svtkParametricFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a Mobius strip with the following parameters:
   * MinimumU = 0, MaximumU = 2*Pi,
   * MinimumV = -1, MaximumV = 1,
   * JoinU = 1, JoinV = 0,
   * TwistU = 0, TwistV = 0,
   * ClockwiseOrdering = 0,
   * DerivativesAvailable = 1,
   * Radius = 1.
   */
  static svtkParametricMobius* New();

  //@{
  /**
   * Set/Get the radius of the Mobius strip. Default is 1.
   */
  svtkSetMacro(Radius, double);
  svtkGetMacro(Radius, double);
  //@}

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  /**
   * The Mobius strip.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$ .
   * Then the normal is \f$N = Du X Dv\f$ .
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) override;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.

   * uvw are the parameters with Pt being the cartesian point,
   * Duvw are the derivatives of this point with respect to u, v and w.
   * Pt, Du, Dv are obtained from Evaluate().

   * This function is only called if the ScalarMode has the value
   * svtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED

   * If the user does not need to calculate a scalar, then the
   * instantiated function should return zero.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) override;

protected:
  svtkParametricMobius();
  ~svtkParametricMobius() override;

  // Variables
  double Radius;

private:
  svtkParametricMobius(const svtkParametricMobius&) = delete;
  void operator=(const svtkParametricMobius&) = delete;
};

#endif
