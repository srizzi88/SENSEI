/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkContourLineInterpolator
 * @brief   Defines API for interpolating/modifying nodes from a svtkContourRepresentation
 *
 * svtkContourLineInterpolator is an abstract base class for interpolators
 * that are used by the svtkContourRepresentation class to interpolate
 * and/or modify nodes in a contour. Subclasses must override the virtual
 * method \c InterpolateLine. This is used by the contour representation
 * to give the interpolator a chance to define an interpolation scheme
 * between nodes. See svtkBezierContourLineInterpolator for a concrete
 * implementation. Subclasses may also override \c UpdateNode. This provides
 * a way for the representation to give the interpolator a chance to modify
 * the nodes, as the user constructs the contours. For instance, a sticky
 * contour widget may be implemented that moves nodes to nearby regions of
 * high gradient, to be used in contour-guided segmentation.
 */

#ifndef svtkContourLineInterpolator_h
#define svtkContourLineInterpolator_h

#include "svtkInteractionWidgetsModule.h" // For export macro
#include "svtkObject.h"

class svtkRenderer;
class svtkContourRepresentation;
class svtkIntArray;

class SVTKINTERACTIONWIDGETS_EXPORT svtkContourLineInterpolator : public svtkObject
{
public:
  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkContourLineInterpolator, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Subclasses that wish to interpolate a line segment must implement this.
   * For instance svtkBezierContourLineInterpolator adds nodes between idx1
   * and idx2, that allow the contour to adhere to a bezier curve.
   */
  virtual int InterpolateLine(
    svtkRenderer* ren, svtkContourRepresentation* rep, int idx1, int idx2) = 0;

  /**
   * The interpolator is given a chance to update the node. For instance, the
   * svtkImageContourLineInterpolator updates the idx'th node in the contour,
   * so it automatically sticks to edges in the vicinity as the user
   * constructs the contour.
   * Returns 0 if the node (world position) is unchanged.
   */
  virtual int UpdateNode(
    svtkRenderer*, svtkContourRepresentation*, double* svtkNotUsed(node), int svtkNotUsed(idx));

  /**
   * Span of the interpolator. ie. the number of control points its supposed
   * to interpolate given a node.

   * The first argument is the current nodeIndex.
   * ie, you'd be trying to interpolate between nodes "nodeIndex" and
   * "nodeIndex-1", unless you're closing the contour in which case, you're
   * trying to interpolate "nodeIndex" and "Node=0".

   * The node span is returned in a svtkIntArray. The default node span is 1
   * (ie. nodeIndices is a 2 tuple (nodeIndex, nodeIndex-1)). However, it
   * need not always be 1. For instance, cubic spline interpolators, which
   * have a span of 3 control points, it can be larger. See
   * svtkBezierContourLineInterpolator for instance.
   */
  virtual void GetSpan(int nodeIndex, svtkIntArray* nodeIndices, svtkContourRepresentation* rep);

protected:
  svtkContourLineInterpolator();
  ~svtkContourLineInterpolator() override;

private:
  svtkContourLineInterpolator(const svtkContourLineInterpolator&) = delete;
  void operator=(const svtkContourLineInterpolator&) = delete;
};

#endif
