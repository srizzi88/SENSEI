/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricDini.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricDini
 * @brief   Generate Dini's surface.
 *
 * svtkParametricDini generates Dini's surface.
 * Dini's surface is a surface that possesses constant negative
 * Gaussian curvature
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

#ifndef svtkParametricDini_h
#define svtkParametricDini_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkParametricFunction.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkParametricDini : public svtkParametricFunction
{
public:
  svtkTypeMacro(svtkParametricDini, svtkParametricFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct Dini's surface with the following parameters:
   * MinimumU = 0, MaximumU = 4*Pi,
   * MinimumV = 0.001, MaximumV = 2,
   * JoinU = 0, JoinV = 0,
   * TwistU = 0, TwistV = 0,
   * ClockwiseOrdering = 0,
   * DerivativesAvailable = 1
   * A = 1, B = 0.2
   */
  static svtkParametricDini* New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  //@{
  /**
   * Set/Get the scale factor.
   * See the definition in Parametric surfaces referred to above.
   * Default is 1.
   */
  svtkSetMacro(A, double);
  svtkGetMacro(A, double);
  //@}

  //@{
  /**
   * Set/Get the scale factor.
   * See the definition in Parametric surfaces referred to above.
   * Default is 0.2
   */
  svtkSetMacro(B, double);
  svtkGetMacro(B, double);
  //@}

  /**
   * Dini's surface.

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
  svtkParametricDini();
  ~svtkParametricDini() override;

  // Variables
  double A;
  double B;

private:
  svtkParametricDini(const svtkParametricDini&) = delete;
  void operator=(const svtkParametricDini&) = delete;
};

#endif
