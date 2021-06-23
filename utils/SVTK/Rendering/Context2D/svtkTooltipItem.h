/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTooltipItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkTooltipItem
 * @brief   takes care of drawing 2D axes
 *
 *
 * The svtkTooltipItem is drawn in screen coordinates. It is used to display a
 * tooltip on a scene, giving additional information about an element on the
 * scene, such as in svtkChartXY. It takes care of ensuring that it draws itself
 * within the bounds of the screen.
 */

#ifndef svtkTooltipItem_h
#define svtkTooltipItem_h

#include "svtkContextItem.h"
#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkStdString.h"                // For svtkStdString ivars
#include "svtkVector.h"                   // Needed for svtkVector2f

class svtkPen;
class svtkBrush;
class svtkTextProperty;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkTooltipItem : public svtkContextItem
{
public:
  svtkTypeMacro(svtkTooltipItem, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static svtkTooltipItem* New();

  //@{
  /**
   * Set the position of the tooltip (in pixels).
   */
  svtkSetVector2Macro(Position, float);
  void SetPosition(const svtkVector2f& pos);
  //@}

  //@{
  /**
   * Get position of the axis (in pixels).
   */
  svtkGetVector2Macro(Position, float);
  svtkVector2f GetPositionVector();
  //@}

  //@{
  /**
   * Get/set the text of the item.
   */
  virtual void SetText(const svtkStdString& title);
  virtual svtkStdString GetText();
  //@}

  //@{
  /**
   * Get a pointer to the svtkTextProperty object that controls the way the
   * text is rendered.
   */
  svtkGetObjectMacro(Pen, svtkPen);
  //@}

  //@{
  /**
   * Get a pointer to the svtkPen object.
   */
  svtkGetObjectMacro(Brush, svtkBrush);
  //@}

  //@{
  /**
   * Get the svtkTextProperty that governs how the tooltip text is displayed.
   */
  svtkGetObjectMacro(TextProperties, svtkTextProperty);
  //@}

  /**
   * Update the geometry of the tooltip.
   */
  void Update() override;

  /**
   * Paint event for the tooltip.
   */
  bool Paint(svtkContext2D* painter) override;

protected:
  svtkTooltipItem();
  ~svtkTooltipItem() override;

  svtkVector2f PositionVector;
  float* Position;
  svtkStdString Text;
  svtkTextProperty* TextProperties;
  svtkPen* Pen;
  svtkBrush* Brush;

private:
  svtkTooltipItem(const svtkTooltipItem&) = delete;
  void operator=(const svtkTooltipItem&) = delete;
};

#endif // svtkTooltipItem_h
