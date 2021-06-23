/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricBoy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricBoy
 * @brief   Generate Boy's surface.
 *
 * svtkParametricBoy generates Boy's surface.
 * This is a Model of the projective plane without singularities.
 * It was found by Werner Boy on assignment from David Hilbert.
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

#ifndef svtkParametricBoy_h
#define svtkParametricBoy_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkParametricFunction.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkParametricBoy : public svtkParametricFunction
{
public:
  svtkTypeMacro(svtkParametricBoy, svtkParametricFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct Boy's surface with the following parameters:
   * MinimumU = 0, MaximumU = Pi,
   * MinimumV = 0, MaximumV = Pi,
   * JoinU = 1, JoinV = 1,
   * TwistU = 1, TwistV = 1;
   * ClockwiseOrdering = 0,
   * DerivativesAvailable = 1,
   * ZScale = 0.125.
   */
  static svtkParametricBoy* New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  //@{
  /**
   * Set/Get the scale factor for the z-coordinate.
   * Default is 1/8, giving a nice shape.
   */
  svtkSetMacro(ZScale, double);
  svtkGetMacro(ZScale, double);
  //@}

  /**
   * Boy's surface.

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
   * Pt, Duvw are obtained from Evaluate().

   * This function is only called if the ScalarMode has the value
   * svtkParametricFunctionSource::SCALAR_FUNCTION_DEFINED

   * If the user does not need to calculate a scalar, then the
   * instantiated function should return zero.
   */
  double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) override;

protected:
  svtkParametricBoy();
  ~svtkParametricBoy() override;

  // Variables
  double ZScale;

private:
  svtkParametricBoy(const svtkParametricBoy&) = delete;
  void operator=(const svtkParametricBoy&) = delete;
};

#endif
