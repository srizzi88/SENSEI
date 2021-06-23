/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChartLegend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkChartLegend
 * @brief   draw the chart legend
 *
 *
 * The svtkChartLegend is drawn in screen coordinates. It is usually one of the
 * last elements of a chart to be drawn. It renders the mark/line for each
 * plot, and the plot labels.
 */

#ifndef svtkChartLegend_h
#define svtkChartLegend_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkContextItem.h"
#include "svtkNew.h"  // For svtkNew
#include "svtkRect.h" // For svtkRectf return value

class svtkChart;
class svtkPen;
class svtkBrush;
class svtkTextProperty;

class SVTKCHARTSCORE_EXPORT svtkChartLegend : public svtkContextItem
{
public:
  svtkTypeMacro(svtkChartLegend, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static svtkChartLegend* New();

  //@{
  /**
   * Set point the legend box is anchored to.
   */
  svtkSetVector2Macro(Point, float);
  //@}

  //@{
  /**
   * Get point the legend box is anchored to.
   */
  svtkGetVector2Macro(Point, float);
  //@}

  enum
  {
    LEFT = 0,
    CENTER,
    RIGHT,
    TOP,
    BOTTOM,
    CUSTOM
  };

  /**
   * Set point the legend box is anchored to.
   */
  void SetPoint(const svtkVector2f& point);

  /**
   * Get point the legend box is anchored to.
   */
  const svtkVector2f& GetPointVector();

  //@{
  /**
   * Set the horizontal alignment of the legend to the point specified.
   * Valid values are LEFT, CENTER and RIGHT.
   */
  svtkSetMacro(HorizontalAlignment, int);
  //@}

  //@{
  /**
   * Get the horizontal alignment of the legend to the point specified.
   */
  svtkGetMacro(HorizontalAlignment, int);
  //@}

  //@{
  /**
   * Set the vertical alignment of the legend to the point specified.
   * Valid values are TOP, CENTER and BOTTOM.
   */
  svtkSetMacro(VerticalAlignment, int);
  //@}

  //@{
  /**
   * Get the vertical alignment of the legend to the point specified.
   */
  svtkGetMacro(VerticalAlignment, int);
  //@}

  //@{
  /**
   * Set the padding between legend marks, default is 5.
   */
  svtkSetMacro(Padding, int);
  //@}

  //@{
  /**
   * Get the padding between legend marks.
   */
  svtkGetMacro(Padding, int);
  //@}

  //@{
  /**
   * Set the symbol width, default is 15.
   */
  svtkSetMacro(SymbolWidth, int);
  //@}

  //@{
  /**
   * Get the legend symbol width.
   */
  svtkGetMacro(SymbolWidth, int);
  //@}

  /**
   * Set the point size of the label text.
   */
  virtual void SetLabelSize(int size);

  /**
   * Get the point size of the label text.
   */
  virtual int GetLabelSize();

  //@{
  /**
   * Get/set if the legend should be drawn inline (inside the chart), or not.
   * True would generally request that the chart draws it inside the chart,
   * false would adjust the chart axes and make space to draw the axes outside.
   */
  svtkSetMacro(Inline, bool);
  svtkGetMacro(Inline, bool);
  //@}

  //@{
  /**
   * Get/set if the legend can be dragged with the mouse button, or not.
   * True results in left click and drag causing the legend to move around the
   * scene. False disables response to mouse events.
   * The default is true.
   */
  svtkSetMacro(DragEnabled, bool);
  svtkGetMacro(DragEnabled, bool);
  //@}

  /**
   * Set the chart that the legend belongs to and will draw the legend for.
   */
  void SetChart(svtkChart* chart);

  /**
   * Get the chart that the legend belongs to and will draw the legend for.
   */
  svtkChart* GetChart();

  /**
   * Update the geometry of the axis. Takes care of setting up the tick mark
   * locations etc. Should be called by the scene before rendering.
   */
  void Update() override;

  /**
   * Paint event for the axis, called whenever the axis needs to be drawn.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Request the space the legend requires to be drawn. This is returned as a
   * svtkRect4f, with the corner being the offset from Point, and the width/
   * height being the total width/height required by the axis. In order to
   * ensure the numbers are correct, Update() should be called first.
   */
  virtual svtkRectf GetBoundingRect(svtkContext2D* painter);

  /**
   * Get the pen used to draw the legend outline.
   */
  svtkPen* GetPen();

  /**
   * Get the brush used to draw the legend background.
   */
  svtkBrush* GetBrush();

  /**
   * Get the svtkTextProperty for the legend's labels.
   */
  svtkTextProperty* GetLabelProperties();

  //@{
  /**
   * Toggle whether or not this legend should attempt to cache its position
   * and size.  The default value is true.  If this value is set to false,
   * the legend will recalculate its position and bounds every time it is
   * drawn.  If users will be able to zoom in or out on your legend, you
   * may want to set this to false.  Otherwise, the border around the legend
   * may not resize appropriately.
   */
  svtkSetMacro(CacheBounds, bool);
  svtkGetMacro(CacheBounds, bool);
  svtkBooleanMacro(CacheBounds, bool);
  //@}

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event
   */
  bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   */
  bool MouseButtonReleaseEvent(const svtkContextMouseEvent& mouse) override;

protected:
  svtkChartLegend();
  ~svtkChartLegend() override;

  float* Point;            // The point the legend is anchored to.
  int HorizontalAlignment; // Alignment of the legend to the point it is anchored to.
  int VerticalAlignment;   // Alignment of the legend to the point it is anchored to.

  /**
   * The pen used to draw the legend box.
   */
  svtkNew<svtkPen> Pen;

  /**
   * The brush used to render the background of the legend.
   */
  svtkNew<svtkBrush> Brush;

  /**
   * The text properties of the labels used in the legend.
   */
  svtkNew<svtkTextProperty> LabelProperties;

  /**
   * Should we move the legend box around in response to the mouse drag?
   */
  bool DragEnabled;

  /**
   * Should the legend attempt to avoid recalculating its position &
   * bounds unnecessarily?
   */
  bool CacheBounds;

  /**
   * Last button to be pressed.
   */
  int Button;

  svtkTimeStamp PlotTime;
  svtkTimeStamp RectTime;

  svtkRectf Rect;

  /**
   * Padding between symbol and text.
   */
  int Padding;

  /**
   * Width of the symbols in pixels in the legend.
   */
  int SymbolWidth;

  /**
   * Should the legend be drawn inline in its chart?
   */
  bool Inline;

  // Private storage class
  class Private;
  Private* Storage;

private:
  svtkChartLegend(const svtkChartLegend&) = delete;
  void operator=(const svtkChartLegend&) = delete;
};

#endif // svtkChartLegend_h
