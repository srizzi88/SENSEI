/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPiecewiseControlPointsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPiecewiseControlPointsItem
 * @brief   Control points for
 * svtkPiecewiseFunction.
 *
 * svtkPiecewiseControlPointsItem draws the control points of a svtkPiecewiseFunction.
 * @sa
 * svtkControlPointsItem
 * svtkPiecewiseFunctionItem
 * svtkCompositeTransferFunctionItem
 */

#ifndef svtkPiecewiseControlPointsItem_h
#define svtkPiecewiseControlPointsItem_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkControlPointsItem.h"

class svtkPiecewiseFunction;

class SVTKCHARTSCORE_EXPORT svtkPiecewiseControlPointsItem : public svtkControlPointsItem
{
public:
  svtkTypeMacro(svtkPiecewiseControlPointsItem, svtkControlPointsItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a piecewise control points object
   */
  static svtkPiecewiseControlPointsItem* New();

  /**
   * Set the piecewise function to draw its points
   */
  virtual void SetPiecewiseFunction(svtkPiecewiseFunction* function);
  //@{
  /**
   * Get the piecewise function
   */
  svtkGetObjectMacro(PiecewiseFunction, svtkPiecewiseFunction);
  //@}

  /**
   * Add a point to the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  svtkIdType AddPoint(double* newPos) override;

  /**
   * Remove a point of the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  svtkIdType RemovePoint(double* pos) override;

  //@{
  /**
   * Controls whether or not control points are drawn (true) or clicked and
   * moved (false).
   * False by default.
   */
  svtkSetMacro(StrokeMode, bool);
  //@}

protected:
  svtkPiecewiseControlPointsItem();
  ~svtkPiecewiseControlPointsItem() override;

  void emitEvent(unsigned long event, void* params = nullptr) override;

  svtkMTimeType GetControlPointsMTime() override;

  svtkIdType GetNumberOfPoints() const override;
  void GetControlPoint(svtkIdType index, double* point) const override;
  void SetControlPoint(svtkIdType index, double* point) override;
  void EditPoint(float tX, float tY) override;

  svtkPiecewiseFunction* PiecewiseFunction;

private:
  svtkPiecewiseControlPointsItem(const svtkPiecewiseControlPointsItem&) = delete;
  void operator=(const svtkPiecewiseControlPointsItem&) = delete;
};

#endif
