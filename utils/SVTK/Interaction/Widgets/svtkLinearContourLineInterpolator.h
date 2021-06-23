/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinearContourLineInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLinearContourLineInterpolator
 * @brief   Interpolates supplied nodes with line segments
 *
 * The line interpolator interpolates supplied nodes (see InterpolateLine)
 * with line segments. The fineness of the curve may be controlled using
 * SetMaximumCurveError and SetMaximumNumberOfLineSegments.
 *
 * @sa
 * svtkContourLineInterpolator
 */

#ifndef svtkLinearContourLineInterpolator_h
#define svtkLinearContourLineInterpolator_h

#include "svtkContourLineInterpolator.h"
#include "svtkInteractionWidgetsModule.h" // For export macro

class SVTKINTERACTIONWIDGETS_EXPORT svtkLinearContourLineInterpolator
  : public svtkContourLineInterpolator
{
public:
  /**
   * Instantiate this class.
   */
  static svtkLinearContourLineInterpolator* New();

  //@{
  /**
   * Standard methods for instances of this class.
   */
  svtkTypeMacro(svtkLinearContourLineInterpolator, svtkContourLineInterpolator);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  int InterpolateLine(svtkRenderer* ren, svtkContourRepresentation* rep, int idx1, int idx2) override;

protected:
  svtkLinearContourLineInterpolator();
  ~svtkLinearContourLineInterpolator() override;

private:
  svtkLinearContourLineInterpolator(const svtkLinearContourLineInterpolator&) = delete;
  void operator=(const svtkLinearContourLineInterpolator&) = delete;
};

#endif
