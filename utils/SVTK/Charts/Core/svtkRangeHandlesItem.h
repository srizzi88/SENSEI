/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRangeHandlesItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkRangeHandlesItem
 * @brief   item to show and control the range of a svtkColorTransferFunction
 *
 * svtkRangeHandlesItem provides range handles painting and management
 * for a provided svtkColorTransferFunction.
 * Handles can be moved by clicking on them.
 * The range is shown when hovering or moving the handles.
 * It emits a StartInteractionEvent when starting to interact with a handle,
 * an InteractionEvent when interacting with a handle and an EndInteractionEvent
 * when releasing a handle.
 * It emits a LeftMouseButtonDoubleClickEvent when double clicked.
 *
 * @sa
 * svtkControlPointsItem
 * svtkScalarsToColorsItem
 * svtkColorTransferFunctionItem
 */

#ifndef svtkRangeHandlesItem_h
#define svtkRangeHandlesItem_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkCommand.h"          // For svtkCommand enum
#include "svtkPlot.h"

class svtkColorTransferFunction;
class svtkBrush;

class SVTKCHARTSCORE_EXPORT svtkRangeHandlesItem : public svtkPlot
{
public:
  svtkTypeMacro(svtkRangeHandlesItem, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkRangeHandlesItem* New();

  enum Handle
  {
    NO_HANDLE = -1,
    LEFT_HANDLE = 0,
    RIGHT_HANDLE = 1
  };

  /**
   * Paint both handles and the range if
   * a handle is active or hovered
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Recover the bounds of the item
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Recover the range currently set by the handles
   * Use this method by observing EndInteractionEvent
   */
  virtual void GetHandlesRange(double range[2]);

  //@{
  /**
   * Get/set the color transfer function to interact with.
   */
  void SetColorTransferFunction(svtkColorTransferFunction* ctf);
  svtkGetObjectMacro(ColorTransferFunction, svtkColorTransferFunction);
  //@}

  //@{
  /**
   * Set/Get the handles width in pixels.
   * Default is 2.
   */
  svtkSetMacro(HandleWidth, float);
  svtkGetMacro(HandleWidth, float);
  //@}

  /**
   * Compute the handles draw range by using the handle width and the transfer function
   */
  void ComputeHandlesDrawRange();

protected:
  svtkRangeHandlesItem();
  ~svtkRangeHandlesItem() override;

  /**
   * Returns true if the supplied x, y coordinate is around a handle
   */
  bool Hit(const svtkContextMouseEvent& mouse) override;

  //@{
  /**
   * Interaction methods to interact with the handles.
   */
  bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse) override;
  bool MouseButtonReleaseEvent(const svtkContextMouseEvent& mouse) override;
  bool MouseMoveEvent(const svtkContextMouseEvent& mouse) override;
  bool MouseEnterEvent(const svtkContextMouseEvent& mouse) override;
  bool MouseLeaveEvent(const svtkContextMouseEvent& mouse) override;
  bool MouseDoubleClickEvent(const svtkContextMouseEvent& mouse) override;
  //@}

  /**
   * Returns the handle the provided point is over with a provided tolerance,
   * it can be NO_HANDLE, LEFT_HANDLE or RIGHT_HANDLE
   */
  virtual int FindRangeHandle(const svtkVector2f& point, const svtkVector2f& tolerance);

  /**
   * Internal method to set the ActiveHandlePosition
   * and compute the ActiveHandleRangeValue accordingly
   */
  void SetActiveHandlePosition(double position);

  /**
   * Internal method to check if the active handle have
   * actually been moved.
   */
  bool IsActiveHandleMoved(double tolerance);

  /**
   * Set the cursor shape
   */
  void SetCursor(int cursor);

private:
  svtkRangeHandlesItem(const svtkRangeHandlesItem&) = delete;
  void operator=(const svtkRangeHandlesItem&) = delete;

  svtkColorTransferFunction* ColorTransferFunction = nullptr;

  float HandleWidth = 2;
  float HandleDelta = 0;
  float LeftHandleDrawRange[2] = { 0, 0 };
  float RightHandleDrawRange[2] = { 0, 0 };
  int ActiveHandle = NO_HANDLE;
  int HoveredHandle = NO_HANDLE;
  double ActiveHandlePosition = 0;
  double ActiveHandleRangeValue = 0;
  svtkNew<svtkBrush> HighlightBrush;
  svtkNew<svtkBrush> RangeLabelBrush;
};

#endif // svtkRangeHandlesItem_h
