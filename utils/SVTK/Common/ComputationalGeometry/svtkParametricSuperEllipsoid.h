/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricSuperEllipsoid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricSuperEllipsoid
 * @brief   Generate a superellipsoid.
 *
 * svtkParametricSuperEllipsoid generates a superellipsoid.  A superellipsoid
 * is a versatile primitive that is controlled by two parameters n1 and
 * n2. As special cases it can represent a sphere, square box, and closed
 * cylindrical can.
 *
 * For further information about this surface, please consult the
 * technical description "Parametric surfaces" in http://www.svtk.org/publications
 * in the "SVTK Technical Documents" section in the VTk.org web pages.
 *
 * Also see: http://paulbourke.net/geometry/superellipse/
 *
 * @warning
 * Care needs to be taken specifying the bounds correctly. You may need to
 * carefully adjust MinimumU, MinimumV, MaximumU, MaximumV.
 *
 * @par Thanks:
 * Andrew Maclean andrew.amaclean@gmail.com for creating and contributing the
 * class.
 *
 */

#ifndef svtkParametricSuperEllipsoid_h
#define svtkParametricSuperEllipsoid_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkParametricFunction.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkParametricSuperEllipsoid
  : public svtkParametricFunction
{
public:
  svtkTypeMacro(svtkParametricSuperEllipsoid, svtkParametricFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a superellipsoid with the following parameters:
   * MinimumU = -Pi, MaximumU = Pi,
   * MinimumV = -Pi/2, MaximumV = Pi/2,
   * JoinU = 0, JoinV = 0,
   * TwistU = 0, TwistV = 0,
   * ClockwiseOrdering = 0,
   * DerivativesAvailable = 0,
   * N1 = 1, N2 = 1, XRadius = 1, YRadius = 1,
   * ZRadius = 1, a sphere in this case.
   */
  static svtkParametricSuperEllipsoid* New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 2; }

  //@{
  /**
   * Set/Get the scaling factor for the x-axis. Default is 1.
   */
  svtkSetMacro(XRadius, double);
  svtkGetMacro(XRadius, double);
  //@}

  //@{
  /**
   * Set/Get the scaling factor for the y-axis. Default is 1.
   */
  svtkSetMacro(YRadius, double);
  svtkGetMacro(YRadius, double);
  //@}

  //@{
  /**
   * Set/Get the scaling factor for the z-axis. Default is 1.
   */
  svtkSetMacro(ZRadius, double);
  svtkGetMacro(ZRadius, double);
  //@}

  //@{
  /**
   * Set/Get the "squareness" parameter in the z axis.  Default is 1.
   */
  svtkSetMacro(N1, double);
  svtkGetMacro(N1, double);
  //@}

  //@{
  /**
   * Set/Get the "squareness" parameter in the x-y plane. Default is 1.
   */
  svtkSetMacro(N2, double);
  svtkGetMacro(N2, double);
  //@}

  /**
   * A superellipsoid.

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
  svtkParametricSuperEllipsoid();
  ~svtkParametricSuperEllipsoid() override;

  // Variables
  double XRadius;
  double YRadius;
  double ZRadius;
  double N1;
  double N2;

private:
  svtkParametricSuperEllipsoid(const svtkParametricSuperEllipsoid&) = delete;
  void operator=(const svtkParametricSuperEllipsoid&) = delete;
};

#endif
