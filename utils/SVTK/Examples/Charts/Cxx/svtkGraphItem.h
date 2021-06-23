/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGraphItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkGraphItem
 * @brief   a svtkContextItem that draws a block (optional label).
 *
 *
 * This is a svtkContextItem that can be placed into a svtkContextScene. It draws
 * a block of the given dimensions, and reacts to mouse events.
 */

#ifndef svtkGraphItem_h
#define svtkGraphItem_h

#include "svtkContextItem.h"

class svtkContext2D;
class svtkGraph;

class svtkGraphItem : public svtkContextItem
{
public:
  svtkTypeMacro(svtkGraphItem, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkGraphItem* New();

  svtkGetObjectMacro(Graph, svtkGraph);
  virtual void SetGraph(svtkGraph* g);

  /**
   * Paint event for the item.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Returns true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse enter event.
   */
  bool MouseEnterEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse leave event.
   */
  bool MouseLeaveEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event.
   */
  bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   */
  bool MouseButtonReleaseEvent(const svtkContextMouseEvent& mouse) override;

  void UpdatePositions();

protected:
  svtkGraphItem();
  ~svtkGraphItem() override;

  float LastPosition[2];

  bool MouseOver;
  int MouseButtonPressed;

  svtkGraph* Graph;
  svtkIdType HitVertex;

  class Implementation;
  Implementation* Impl;

private:
  svtkGraphItem(const svtkGraphItem&) = delete;
  void operator=(const svtkGraphItem&) = delete;
};

#endif // svtkGraphItem_h
