/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBezierContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBezierContourLineInterpolator
 * @brief   Interpolates supplied nodes with bezier line segments
 *
 * The line interpolator interpolates supplied nodes (see InterpolateLine)
 * with Bezier line segments. The fitness of the curve may be controlled using
 * SetMaximumCurveError and SetMaximumNumberOfLineSegments.
 *
 * @sa
 * svtkContourLineInterpolator
 */

#ifndef svtkBezierContourLineInterpolator_h
#define svtkBezierContourLineInterpolator_h

#include "svtkContourLineInterpolator.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class SVTKINTERACTIONWIDGETS_EXPORT svtkBezierContourLineInterpolator
  : public svtkContourLineInterpolator
{
public:
  /**
   * Instantiate this class.
   */
  static svtkBezierContourLineInterpolator* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkBezierContourLineInterpolator, svtkContourLineInterpolator);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  int InterpolateLine(svtkRenderer* ren, svtkContourRepresentation* rep, int idx1, int idx2) override;

  //@{
  /**
   * The difference between a line segment connecting two points and the curve
   * connecting the same points. In the limit of the length of the curve
   * dx -> 0, the two values will be the same. The smaller this number, the
   * finer the bezier curve will be interpolated. Default is 0.005
   */
  svtkSetClampMacro(MaximumCurveError, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(MaximumCurveError, double);
  //@}

  //@{
  /**
   * Maximum number of bezier line segments between two nodes. Larger values
   * create a finer interpolation. Default is 100.
   */
  svtkSetClampMacro(MaximumCurveLineSegments, int, 1, 1000);
  svtkGetMacro(MaximumCurveLineSegments, int);
  //@}

  /**
   * Span of the interpolator, i.e. the number of control points it's supposed
   * to interpolate given a node.

   * The first argument is the current nodeIndex.
   * i.e., you'd be trying to interpolate between nodes "nodeIndex" and
   * "nodeIndex-1", unless you're closing the contour, in which case you're
   * trying to interpolate "nodeIndex" and "Node=0". The node span is
   * returned in a svtkIntArray.

   * The node span is returned in a svtkIntArray. The node span returned by
   * this interpolator will be a 2-tuple with a span of 4.
   */
  void GetSpan(int nodeIndex, svtkIntArray* nodeIndices, svtkContourRepresentation* rep) override;

protected:
  svtkBezierContourLineInterpolator();
  ~svtkBezierContourLineInterpolator() override;

  void ComputeMidpoint(double p1[3], double p2[3], double mid[3])
  {
    mid[0] = (p1[0] + p2[0]) / 2;
    mid[1] = (p1[1] + p2[1]) / 2;
    mid[2] = (p1[2] + p2[2]) / 2;
  }

  double MaximumCurveError;
  int MaximumCurveLineSegments;

private:
  svtkBezierContourLineInterpolator(const svtkBezierContourLineInterpolator&) = delete;
  void operator=(const svtkBezierContourLineInterpolator&) = delete;
};

#endif
