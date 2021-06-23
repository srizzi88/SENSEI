/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkKochanekSpline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkKochanekSpline
 * @brief   computes an interpolating spline using a Kochanek basis.
 *
 * Implements the Kochanek interpolating spline described in: Kochanek, D.,
 * Bartels, R., "Interpolating Splines with Local Tension, Continuity, and
 * Bias Control," Computer Graphics, vol. 18, no. 3, pp. 33-41, July 1984.
 * These splines give the user more control over the shape of the curve than
 * the cardinal splines implemented in svtkCardinalSpline. Three parameters
 * can be specified. All have a range from -1 to 1.
 *
 * Tension controls how sharply the curve bends at an input point. A
 * value of -1 produces more slack in the curve. A value of 1 tightens
 * the curve.
 *
 * Continuity controls the continuity of the first derivative at input
 * points.
 *
 * Bias controls the direction of the curve at it passes through an input
 * point. A value of -1 undershoots the point while a value of 1
 * overshoots the point.
 *
 * These three parameters give the user broad control over the shape of
 * the interpolating spline. The original Kochanek paper describes the
 * effects nicely and is recommended reading.
 *
 * @sa
 * svtkSpline svtkCardinalSpline
 */

#ifndef svtkKochanekSpline_h
#define svtkKochanekSpline_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkSpline.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkKochanekSpline : public svtkSpline
{
public:
  svtkTypeMacro(svtkKochanekSpline, svtkSpline);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct a KochanekSpline with the following defaults: DefaultBias = 0,
   * DefaultTension = 0, DefaultContinuity = 0.
   */
  static svtkKochanekSpline* New();

  /**
   * Compute Kochanek Spline coefficients.
   */
  void Compute() override;

  /**
   * Evaluate a 1D Kochanek spline.
   */
  double Evaluate(double t) override;

  //@{
  /**
   * Set the bias for all points. Default is 0.
   */
  svtkSetMacro(DefaultBias, double);
  svtkGetMacro(DefaultBias, double);
  //@}

  //@{
  /**
   * Set the tension for all points. Default is 0.
   */
  svtkSetMacro(DefaultTension, double);
  svtkGetMacro(DefaultTension, double);
  //@}

  //@{
  /**
   * Set the continuity for all points. Default is 0.
   */
  svtkSetMacro(DefaultContinuity, double);
  svtkGetMacro(DefaultContinuity, double);
  //@}

  /**
   * Deep copy of cardinal spline data.
   */
  void DeepCopy(svtkSpline* s) override;

protected:
  svtkKochanekSpline();
  ~svtkKochanekSpline() override {}

  void Fit1D(int n, double* x, double* y, double tension, double bias, double continuity,
    double coefficients[][4], int leftConstraint, double leftValue, int rightConstraint,
    double rightValue);

  double DefaultBias;
  double DefaultTension;
  double DefaultContinuity;

private:
  svtkKochanekSpline(const svtkKochanekSpline&) = delete;
  void operator=(const svtkKochanekSpline&) = delete;
};

#endif
