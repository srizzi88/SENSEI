/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSCurveSpline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   svtkSCurveSpline
 * @brief   computes an interpolating spline using a
 * a SCurve basis.
 *
 *
 * svtkSCurveSpline is a concrete implementation of svtkSpline using a
 * SCurve basis.
 *
 * @sa
 * svtkSpline svtkKochanekSpline
 */

#ifndef svtkSCurveSpline_h
#define svtkSCurveSpline_h

#include "svtkSpline.h"
#include "svtkViewsInfovisModule.h" // For export macro

class SVTKVIEWSINFOVIS_EXPORT svtkSCurveSpline : public svtkSpline
{
public:
  static svtkSCurveSpline* New();

  svtkTypeMacro(svtkSCurveSpline, svtkSpline);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Compute SCurve Splines for each dependent variable
   */
  void Compute() override;

  /**
   * Evaluate a 1D SCurve spline.
   */
  double Evaluate(double t) override;

  /**
   * Deep copy of SCurve spline data.
   */
  void DeepCopy(svtkSpline* s) override;

  svtkSetMacro(NodeWeight, double);
  svtkGetMacro(NodeWeight, double);

protected:
  svtkSCurveSpline();
  ~svtkSCurveSpline() override {}

  double NodeWeight;

private:
  svtkSCurveSpline(const svtkSCurveSpline&) = delete;
  void operator=(const svtkSCurveSpline&) = delete;
};

#endif
