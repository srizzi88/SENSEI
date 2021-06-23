/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPiecewisePointHandleItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPiecewisePointHandleItem
 * @brief   a svtkContextItem that draws handles
 *       around a point of a piecewise function
 *
 *
 * This is a svtkContextItem that can be placed into a svtkContextScene. It draws
 * handles around a given point of a piecewise function so that the curve can
 * be adjusted using these handles.
 */

#ifndef svtkPiecewisePointHandleItem_h
#define svtkPiecewisePointHandleItem_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkContextItem.h"
#include "svtkWeakPointer.h" // Needed for weak pointer to the PiecewiseFunction.

class svtkContext2D;
class svtkPiecewiseFunction;
class svtkCallbackCommand;
class svtkAbstractContextItem;

class SVTKCHARTSCORE_EXPORT svtkPiecewisePointHandleItem : public svtkContextItem
{
public:
  svtkTypeMacro(svtkPiecewisePointHandleItem, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPiecewisePointHandleItem* New();
  static void CallRedraw(svtkObject* sender, unsigned long event, void* receiver, void* params);

  /**
   * Set the parent item, which should be a svtkControlPointItem
   */
  void SetParent(svtkAbstractContextItem* parent) override;

  /**
   * Paint event for the item.
   */
  bool Paint(svtkContext2D* painter) override;

  //@{
  /**
   * The current point id in the piecewise function being handled.
   */
  svtkSetMacro(CurrentPointIndex, svtkIdType);
  svtkGetMacro(CurrentPointIndex, svtkIdType);
  //@}

  //@{
  /**
   * Set the PieceWiseFunction the handles will manipulate
   */
  virtual void SetPiecewiseFunction(svtkPiecewiseFunction* piecewiseFunc);
  svtkWeakPointer<svtkPiecewiseFunction> GetPiecewiseFunction();
  //@}

  /**
   * Returns the index of the handle if pos is over any of the handles,
   * otherwise return -1;
   */
  int IsOverHandle(float* pos);

  /**
   * Returns true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event.
   */
  bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   */
  bool MouseButtonReleaseEvent(const svtkContextMouseEvent& mouse) override;

protected:
  svtkPiecewisePointHandleItem();
  ~svtkPiecewisePointHandleItem() override;

  /**
   * Redraw all the handles
   */
  virtual void Redraw();

  int MouseOverHandleIndex;
  svtkIdType CurrentPointIndex;
  float HandleRadius;

  svtkWeakPointer<svtkPiecewiseFunction> PiecewiseFunction;
  svtkCallbackCommand* Callback;

private:
  svtkPiecewisePointHandleItem(const svtkPiecewisePointHandleItem&) = delete;
  void operator=(const svtkPiecewisePointHandleItem&) = delete;

  class InternalPiecewisePointHandleInfo;
  InternalPiecewisePointHandleInfo* Internal;
};

#endif // svtkPiecewisePointHandleItem_h
