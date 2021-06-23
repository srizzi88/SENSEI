/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBlockItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkBlockItem
 * @brief   a svtkContextItem that draws a block (optional label).
 *
 *
 * This is a svtkContextItem that can be placed into a svtkContextScene. It draws
 * a block of the given dimensions, and reacts to mouse events.
 */

#ifndef svtkBlockItem_h
#define svtkBlockItem_h

#include "svtkContextItem.h"
#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkStdString.h"                // For svtkStdString ivars

class svtkContext2D;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkBlockItem : public svtkContextItem
{
public:
  svtkTypeMacro(svtkBlockItem, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkBlockItem* New();

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

  /**
   * Set the block label.
   */
  virtual void SetLabel(const svtkStdString& label);

  /**
   * Get the block label.
   */
  virtual svtkStdString GetLabel();

  //@{
  /**
   * Set the dimensions of the block, elements 0 and 1 are the x and y
   * coordinate of the bottom corner. Elements 2 and 3 are the width and
   * height.
   * Initial value is (0,0,0,0).
   */
  svtkSetVector4Macro(Dimensions, float);
  //@}

  //@{
  /**
   * Get the dimensions of the block, elements 0 and 1 are the x and y
   * coordinate of the bottom corner. Elements 2 and 3 are the width and
   * height.
   * Initial value is (0,0,0,0)
   */
  svtkGetVector4Macro(Dimensions, float);
  //@}

  void SetScalarFunctor(double (*scalarFunction)(double, double));

protected:
  svtkBlockItem();
  ~svtkBlockItem() override;

  float Dimensions[4];

  svtkStdString Label;

  bool MouseOver;

  // Some function pointers to optionally do funky things...
  double (*scalarFunction)(double, double);

private:
  svtkBlockItem(const svtkBlockItem&) = delete;
  void operator=(const svtkBlockItem&) = delete;
};

#endif // svtkBlockItem_h
