/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricSpline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricSpline
 * @brief   parametric function for 1D interpolating splines
 *
 * svtkParametricSpline is a parametric function for 1D interpolating splines.
 * svtkParametricSpline maps the single parameter u into a 3D point (x,y,z)
 * using three instances of interpolating splines.  This family of 1D splines
 * is guaranteed to be parameterized in the interval [0,1].  Attempting to
 * evaluate outside this interval will cause the parameter u to be clamped in
 * the range [0,1].
 *
 * When constructed, this class creates instances of svtkCardinalSpline for
 * each of the x-y-z coordinates. The user may choose to replace these with
 * their own instances of subclasses of svtkSpline.
 *
 * @warning
 * If you wish to tessellate the spline, use the class
 * svtkParametricFunctionSource.
 *
 * @sa
 * svtkSpline svtkKochanekSpline svtkCardinalSpline
 */

#ifndef svtkParametricSpline_h
#define svtkParametricSpline_h

class svtkSpline;
class svtkPoints;

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkParametricFunction.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkParametricSpline : public svtkParametricFunction
{
public:
  svtkTypeMacro(svtkParametricSpline, svtkParametricFunction);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct the spline with the following parameters:
   * MinimumU = 0, MaximumU = 1, JoinU = 0 (unless the spline is
   * closed, then JoinU = 1), TwistU = 0, DerivativesSupplied = 0
   * (the other svtkParametricFunction parameters are ignored).
   */
  static svtkParametricSpline* New();

  /**
   * Return the parametric dimension of the class.
   */
  int GetDimension() override { return 1; }

  /**
   * Evaluate the spline at parametric coordinate u[0] returning
   * the point coordinate Pt[3].
   */
  void Evaluate(double u[3], double Pt[3], double Du[9]) override;

  /**
   * Evaluate a scalar value at parametric coordinate u[0] and Pt[3].
   * The scalar value is just the parameter u[0].
   */
  double EvaluateScalar(double u[3], double Pt[3], double Du[9]) override;

  //@{
  /**
   * By default, this class is constructed with three instances of
   * svtkCardinalSpline (for each of the x-y-z coordinate axes). The user may
   * choose to create and assign their own instances of svtkSpline.
   */
  void SetXSpline(svtkSpline*);
  void SetYSpline(svtkSpline*);
  void SetZSpline(svtkSpline*);
  svtkGetObjectMacro(XSpline, svtkSpline);
  svtkGetObjectMacro(YSpline, svtkSpline);
  svtkGetObjectMacro(ZSpline, svtkSpline);
  //@}

  //@{
  /**
   * Specify the list of points defining the spline. Do this by
   * specifying a svtkPoints array containing the points. Note that
   * the order of the points in svtkPoints is the order that the
   * splines will be fit.
   */
  void SetPoints(svtkPoints*);
  svtkGetObjectMacro(Points, svtkPoints);
  //@}

  //@{
  /**
   * Another API to set the points. Set the number of points and then set the
   * individual point coordinates.
   */
  void SetNumberOfPoints(svtkIdType numPts);
  void SetPoint(svtkIdType index, double x, double y, double z);
  //@}

  //@{
  /**
   * Control whether the spline is open or closed. A closed spline forms
   * a continuous loop: the first and last points are the same, and
   * derivatives are continuous.
   */
  svtkSetMacro(Closed, svtkTypeBool);
  svtkGetMacro(Closed, svtkTypeBool);
  svtkBooleanMacro(Closed, svtkTypeBool);
  //@}

  //@{
  /**
   * Control whether the spline is parameterized by length or by point index.
   * Default is by length.
   */
  svtkSetMacro(ParameterizeByLength, svtkTypeBool);
  svtkGetMacro(ParameterizeByLength, svtkTypeBool);
  svtkBooleanMacro(ParameterizeByLength, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the type of constraint of the left(right) end points. Four
   * constraints are available:

   * 0: the first derivative at left(right) most point is determined
   * from the line defined from the first(last) two points.

   * 1: the first derivative at left(right) most point is set to
   * Left(Right)Value.

   * 2: the second derivative at left(right) most point is set to
   * Left(Right)Value.

   * 3: the second derivative at left(right)most points is Left(Right)Value
   * times second derivative at first interior point.
   */
  svtkSetClampMacro(LeftConstraint, int, 0, 3);
  svtkGetMacro(LeftConstraint, int);
  svtkSetClampMacro(RightConstraint, int, 0, 3);
  svtkGetMacro(RightConstraint, int);
  //@}

  //@{
  /**
   * The values of the derivative on the left and right sides. The value
   * is used only if the left(right) constraint is type 1-3.
   */
  svtkSetMacro(LeftValue, double);
  svtkGetMacro(LeftValue, double);
  svtkSetMacro(RightValue, double);
  svtkGetMacro(RightValue, double);
  //@}

protected:
  svtkParametricSpline();
  ~svtkParametricSpline() override;

  // Points definition
  svtkPoints* Points;

  // The interpolating splines for each of the x-y-z coordinates
  svtkSpline* XSpline;
  svtkSpline* YSpline;
  svtkSpline* ZSpline;

  // Supplemental variables
  svtkTypeBool Closed;
  int LeftConstraint;
  int RightConstraint;
  double LeftValue;
  double RightValue;
  svtkTypeBool ParameterizeByLength;

  // Initializing the spline
  svtkMTimeType InitializeTime;
  int Initialize();

  // Internal variable for managing parametric coordinates
  double Length;
  double ClosedLength;

private:
  svtkParametricSpline(const svtkParametricSpline&) = delete;
  void operator=(const svtkParametricSpline&) = delete;
};

#endif
