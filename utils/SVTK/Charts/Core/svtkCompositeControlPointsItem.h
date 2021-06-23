/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeControlPointsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkCompositeControlPointsItem
 * @brief   Control points for
 * svtkCompositeFunction.
 *
 * svtkCompositeControlPointsItem draws the control points of a svtkPiecewiseFunction
 * and a svtkColorTransferFunction.
 * @sa
 * svtkControlPointsItem
 * svtkColorTransferControlPointsItem
 * svtkCompositeTransferFunctionItem
 * svtkPiecewisePointHandleItem
 */

#ifndef svtkCompositeControlPointsItem_h
#define svtkCompositeControlPointsItem_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkColorTransferControlPointsItem.h"

class svtkPiecewiseFunction;
class svtkPiecewisePointHandleItem;

class SVTKCHARTSCORE_EXPORT svtkCompositeControlPointsItem : public svtkColorTransferControlPointsItem
{
public:
  svtkTypeMacro(svtkCompositeControlPointsItem, svtkColorTransferControlPointsItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a piecewise control points object
   */
  static svtkCompositeControlPointsItem* New();

  /**
   * Set the color transfer function to draw its points
   */
  virtual void SetColorTransferFunction(svtkColorTransferFunction* function);

  //@{
  /**
   * Utility function that calls SetPiecewiseFunction()
   */
  void SetOpacityFunction(svtkPiecewiseFunction* opacity);
  svtkGetObjectMacro(OpacityFunction, svtkPiecewiseFunction);
  //@}

  enum PointsFunctionType
  {
    ColorPointsFunction = 1,
    OpacityPointsFunction = 2,
    ColorAndOpacityPointsFunction = 3
  };
  //@{
  /**
   * PointsFunction controls whether the points represent the
   * ColorTransferFunction, OpacityTransferFunction or both.
   * If ColorPointsFunction, only the points of the ColorTransfer function are
   * used.
   * If OpacityPointsFunction, only the points of the Opacity function are used
   * If ColorAndOpacityPointsFunction, the points of both functions are shared
   * by both functions.
   * ColorAndOpacityPointsFunction by default.
   * Note: Set the mode before the functions are set. ColorPointsFunction is
   * not fully supported.
   */
  svtkSetMacro(PointsFunction, int);
  svtkGetMacro(PointsFunction, int);
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
   * If UseOpacityPointHandles is true, when the current point is
   * double clicked, a svtkPiecewisePointHandleItem will show up so
   * that the sharpness and mid point can be adjusted in the scene
   * with those handles
   * False by default.
   */
  svtkSetMacro(UseOpacityPointHandles, bool);
  svtkGetMacro(UseOpacityPointHandles, bool);
  //@}

  //@{
  /**
   * Mouse move event. To take care of some special Key stroke
   */
  bool MouseMoveEvent(const svtkContextMouseEvent& mouse) override;
  bool MouseDoubleClickEvent(const svtkContextMouseEvent& mouse) override;
  bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse) override;
  //@}

protected:
  svtkCompositeControlPointsItem();
  ~svtkCompositeControlPointsItem() override;

  void emitEvent(unsigned long event, void* params) override;

  svtkMTimeType GetControlPointsMTime() override;

  svtkIdType GetNumberOfPoints() const override;
  void DrawPoint(svtkContext2D* painter, svtkIdType index) override;
  void GetControlPoint(svtkIdType index, double* pos) const override;
  void SetControlPoint(svtkIdType index, double* point) override;
  void EditPoint(float tX, float tY) override;
  virtual void EditPointCurve(svtkIdType idx);

  void MergeTransferFunctions();
  void SilentMergeTransferFunctions();

  int PointsFunction;
  svtkPiecewiseFunction* OpacityFunction;
  svtkPiecewisePointHandleItem* OpacityPointHandle;
  bool UseOpacityPointHandles;

private:
  svtkCompositeControlPointsItem(const svtkCompositeControlPointsItem&) = delete;
  void operator=(const svtkCompositeControlPointsItem&) = delete;
};

#endif
