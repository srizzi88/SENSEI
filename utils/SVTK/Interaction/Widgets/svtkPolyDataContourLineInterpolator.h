/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataContourLineInterpolator
 * @brief   Contour interpolator for polygonal data
 *
 *
 * svtkPolyDataContourLineInterpolator is an abstract base class for contour
 * line interpolators that interpolate on polygonal data.
 *
 */

#ifndef svtkPolyDataContourLineInterpolator_h
#define svtkPolyDataContourLineInterpolator_h

#include "svtkContourLineInterpolator.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class svtkPolyData;
class svtkPolyDataCollection;

class SVTKINTERACTIONWIDGETS_EXPORT svtkPolyDataContourLineInterpolator
  : public svtkContourLineInterpolator
{
public:
  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkPolyDataContourLineInterpolator, svtkContourLineInterpolator);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Subclasses that wish to interpolate a line segment must implement this.
   * For instance svtkBezierContourLineInterpolator adds nodes between idx1
   * and idx2, that allow the contour to adhere to a bezier curve.
   */
  int InterpolateLine(
    svtkRenderer* ren, svtkContourRepresentation* rep, int idx1, int idx2) override = 0;

  /**
   * The interpolator is given a chance to update the node.
   * svtkImageContourLineInterpolator updates the idx'th node in the contour,
   * so it automatically sticks to edges in the vicinity as the user
   * constructs the contour.
   * Returns 0 if the node (world position) is unchanged.
   */
  int UpdateNode(svtkRenderer*, svtkContourRepresentation*, double* svtkNotUsed(node),
    int svtkNotUsed(idx)) override = 0;

  //@{
  /**
   * Be sure to add polydata on which you wish to place points to this list
   * or they will not be considered for placement.
   */
  svtkGetObjectMacro(Polys, svtkPolyDataCollection);
  //@}

protected:
  svtkPolyDataContourLineInterpolator();
  ~svtkPolyDataContourLineInterpolator() override;

  svtkPolyDataCollection* Polys;

private:
  svtkPolyDataContourLineInterpolator(const svtkPolyDataContourLineInterpolator&) = delete;
  void operator=(const svtkPolyDataContourLineInterpolator&) = delete;
};

#endif
