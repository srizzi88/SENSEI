/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricTorus.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricTorus
 * @brief   Generate a torus.
 *
 * svtkParametricTorus generates a torus.
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

#ifndef svtkParametricTorus_h
#define svtkParametricTorus_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkParametricFunction.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkParametricTorus : public svtkParametricFunction
{

public:
  svtkTypeMacro(svtkParametricTorus, svtkParametricFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a torus with the following parameters:
   * MinimumU = 0, MaximumU = 2*Pi,
   * MinimumV = 0, MaximumV = 2*Pi,
   * JoinU = 1, JoinV = 1,
   * TwistU = 0, TwistV = 0,
   * ClockwiseOrdering = 1,
   * DerivativesAvailable = 0,
   * RingRadius = 1, CrossSectionRadius = 0.5.
   */
  static svtkParametricTorus* New();

  //@{
  /**
   * Set/Get the radius from the center to the middle of the ring of the
   * torus. Default is 1.0.
   */
  svtkSetMacro(RingRadius, double);
  svtkGetMacro(RingRadius, double);
  //@}

  //@{
  /**
   * Set/Get the radius of the cross section of ring of the torus. Default is 0.5.
   */
  svtkSetMacro(CrossSectionRadius, double);
  svtkGetMacro(CrossSectionRadius, double);
  //@}

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  /**
   * A torus.

   * This function performs the mapping \f$f(u,v) \rightarrow (x,y,x)\f$, returning it
   * as Pt. It also returns the partial derivatives Du and Dv.
   * \f$Pt = (x, y, z), Du = (dx/du, dy/du, dz/du), Dv = (dx/dv, dy/dv, dz/dv)\f$.
   * Then the normal is \f$N = Du X Dv\f$.
   */
  void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) override;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.

   * uvw are the parameters with Pt being the Cartesian point,
   * Duvw are the derivatives of this point with respect to u, v and w.
   * Pt, Duvw are obtained from Evaluate().

   * This function is only called if the ScalarMode has the value
   * svtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED

   * If the user does not need to calculate a scalar, then the
   * instantiated function should return zero.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) override;

protected:
  svtkParametricTorus();
  ~svtkParametricTorus() override;

  // Variables
  double RingRadius;
  double CrossSectionRadius;

private:
  svtkParametricTorus(const svtkParametricTorus&) = delete;
  void operator=(const svtkParametricTorus&) = delete;
};

#endif
