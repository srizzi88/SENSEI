/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCardinalSpline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCardinalSpline
 * @brief   computes an interpolating spline using a
 * a Cardinal basis.
 *
 *
 * svtkCardinalSpline is a concrete implementation of svtkSpline using a
 * Cardinal basis.
 *
 * @sa
 * svtkSpline svtkKochanekSpline
 */

#ifndef svtkCardinalSpline_h
#define svtkCardinalSpline_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkSpline.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkCardinalSpline : public svtkSpline
{
public:
  static svtkCardinalSpline* New();

  svtkTypeMacro(svtkCardinalSpline, svtkSpline);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Compute Cardinal Splines for each dependent variable
   */
  void Compute() override;

  /**
   * Evaluate a 1D cardinal spline.
   */
  double Evaluate(double t) override;

  /**
   * Deep copy of cardinal spline data.
   */
  void DeepCopy(svtkSpline* s) override;

protected:
  svtkCardinalSpline();
  ~svtkCardinalSpline() override {}

  void Fit1D(int n, double* x, double* y, double* w, double coefficients[][4], int leftConstraint,
    double leftValue, int rightConstraint, double rightValue);

  void FitClosed1D(int n, double* x, double* y, double* w, double coefficients[][4]);

private:
  svtkCardinalSpline(const svtkCardinalSpline&) = delete;
  void operator=(const svtkCardinalSpline&) = delete;
};

#endif
