/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkColorTransferControlPointsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkColorTransferControlPointsItem
 * @brief   Control points for
 * svtkColorTransferFunction.
 *
 * svtkColorTransferControlPointsItem draws the control points of a svtkColorTransferFunction.
 * @sa
 * svtkControlPointsItem
 * svtkColorTransferFunctionItem
 * svtkCompositeTransferFunctionItem
 */

#ifndef svtkColorTransferControlPointsItem_h
#define svtkColorTransferControlPointsItem_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkControlPointsItem.h"

class svtkColorTransferFunction;

class SVTKCHARTSCORE_EXPORT svtkColorTransferControlPointsItem : public svtkControlPointsItem
{
public:
  svtkTypeMacro(svtkColorTransferControlPointsItem, svtkControlPointsItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a piecewise control points object
   */
  static svtkColorTransferControlPointsItem* New();

  /**
   * Set the piecewise function to draw its points
   */
  void SetColorTransferFunction(svtkColorTransferFunction* function);
  //@{
  /**
   * Get the piecewise function
   */
  svtkGetObjectMacro(ColorTransferFunction, svtkColorTransferFunction);
  //@}

  /**
   * Return the number of points in the color transfer function.
   */
  svtkIdType GetNumberOfPoints() const override;

  /**
   * Returns the x and y coordinates as well as the midpoint and sharpness
   * of the control point corresponding to the index.
   * Note: The y (point[1]) is always 0.5
   */
  void GetControlPoint(svtkIdType index, double* point) const override;

  /**
   * Sets the x and y coordinates as well as the midpoint and sharpness
   * of the control point corresponding to the index.
   * Changing the y has no effect, it will always be 0.5
   */
  void SetControlPoint(svtkIdType index, double* point) override;

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
   * If ColorFill is true, the control point brush color is set with the
   * matching color in the color transfer function.
   * False by default.
   */
  svtkSetMacro(ColorFill, bool);
  svtkGetMacro(ColorFill, bool);
  //@}

protected:
  svtkColorTransferControlPointsItem();
  ~svtkColorTransferControlPointsItem() override;

  void emitEvent(unsigned long event, void* params) override;

  svtkMTimeType GetControlPointsMTime() override;

  void DrawPoint(svtkContext2D* painter, svtkIdType index) override;
  void EditPoint(float tX, float tY) override;

  /**
   * Compute the bounds for this item. Overridden to use the
   * svtkColorTransferFunction range.
   */
  void ComputeBounds(double* bounds) override;

  svtkColorTransferFunction* ColorTransferFunction;

  bool ColorFill;

private:
  svtkColorTransferControlPointsItem(const svtkColorTransferControlPointsItem&) = delete;
  void operator=(const svtkColorTransferControlPointsItem&) = delete;
};

#endif
