/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChartXY.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkChartXY
 * @brief   Factory class for drawing XY charts
 *
 *
 * This class implements an XY chart.
 *
 * @sa
 * svtkBarChartActor
 */

#ifndef svtkChartXY_h
#define svtkChartXY_h

#include "svtkChart.h"
#include "svtkChartsCoreModule.h" // For export macro
#include "svtkContextPolygon.h"   // For svtkContextPolygon
#include "svtkSmartPointer.h"     // For SP ivars
#include "svtkVector.h"           // For svtkVector2f in struct

class svtkAxis;
class svtkChartLegend;
class svtkIdTypeArray;
class svtkPlot;
class svtkPlotGrid;
class svtkTooltipItem;

class svtkChartXYPrivate; // Private class to keep my STL vector in...

class SVTKCHARTSCORE_EXPORT svtkChartXY : public svtkChart
{
public:
  svtkTypeMacro(svtkChartXY, svtkChart);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static svtkChartXY* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the chart, called whenever the chart needs to be drawn
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Add a plot to the chart, defaults to using the name of the y column
   */
  svtkPlot* AddPlot(int type) override;

  /**
   * Adds a plot to the chart
   */
  svtkIdType AddPlot(svtkPlot* plot) override;

  /**
   * Remove the plot at the specified index, returns true if successful,
   * false if the index was invalid.
   */
  bool RemovePlot(svtkIdType index) override;

  /**
   * Remove all plots from the chart.
   */
  void ClearPlots() override;

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  svtkPlot* GetPlot(svtkIdType index) override;

  /**
   * Get the index of the specified plot, returns -1 if the plot does not
   * belong to the chart.
   */
  virtual svtkIdType GetPlotIndex(svtkPlot*);

  /**
   * Raises the \a plot to the top of the plot's stack.
   * \return The new index of the plot
   * \sa StackPlotAbove(), LowerPlot(), StackPlotUnder()
   */
  svtkIdType RaisePlot(svtkPlot* plot);

  /**
   * Raises the \a plot above the \a under plot. If \a under is null,
   * the plot is raised to the top of the plot's stack.
   * \return The new index of the plot
   * \sa RaisePlot(), LowerPlot(), StackPlotUnder()
   */
  virtual svtkIdType StackPlotAbove(svtkPlot* plot, svtkPlot* under);

  /**
   * Lowers the \a plot to the bottom of the plot's stack.
   * \return The new index of the plot
   * \sa StackPlotUnder(), RaisePlot(), StackPlotAbove()
   */
  svtkIdType LowerPlot(svtkPlot* plot);

  /**
   * Lowers the \a plot under the \a above plot. If \a above is null,
   * the plot is lowered to the bottom of the plot's stack
   * \return The new index of the plot
   * \sa StackPlotUnder(), RaisePlot(), StackPlotAbove()
   */
  virtual svtkIdType StackPlotUnder(svtkPlot* plot, svtkPlot* above);

  /**
   * Get the number of plots the chart contains.
   */
  svtkIdType GetNumberOfPlots() override;

  /**
   * Figure out which quadrant the plot is in.
   */
  int GetPlotCorner(svtkPlot* plot);

  /**
   * Figure out which quadrant the plot is in.
   */
  void SetPlotCorner(svtkPlot* plot, int corner);

  /**
   * Get the axis specified by axisIndex. This is specified with the svtkAxis
   * position enum, valid values are svtkAxis::LEFT, svtkAxis::BOTTOM,
   * svtkAxis::RIGHT and svtkAxis::TOP.
   */
  svtkAxis* GetAxis(int axisIndex) override;

  /**
   * Set the axis specified by axisIndex. This is specified with the svtkAxis
   * position enum, valid values are svtkAxis::LEFT, svtkAxis::BOTTOM,
   * svtkAxis::RIGHT and svtkAxis::TOP.
   */
  virtual void SetAxis(int axisIndex, svtkAxis*) override;

  /**
   * Set whether the chart should draw a legend.
   */
  void SetShowLegend(bool visible) override;

  /**
   * Get the svtkChartLegend object that will be displayed by the chart.
   */
  svtkChartLegend* GetLegend() override;

  /**
   * Set the svtkTooltipItem object that will be displayed by the chart.
   */
  virtual void SetTooltip(svtkTooltipItem* tooltip);

  /**
   * Get the svtkTooltipItem object that will be displayed by the chart.
   */
  virtual svtkTooltipItem* GetTooltip();

  /**
   * Get the number of axes in the current chart.
   */
  svtkIdType GetNumberOfAxes() override;

  /**
   * Request that the chart recalculates the range of its axes. Especially
   * useful in applications after the parameters of plots have been modified.
   */
  void RecalculateBounds() override;

  /**
   * Set the selection method, which controls how selections are handled by the
   * chart. The default is SELECTION_ROWS which selects all points in all plots
   * in a chart that have values in the rows selected. SELECTION_PLOTS allows
   * for finer-grained selections specific to each plot, and so to each XY
   * column pair.
   */
  void SetSelectionMethod(int method) override;

  /**
   * Remove all the selection from Plots
   */
  void RemovePlotSelections();

  //@{
  /**
   * If true then the axes will be drawn at the origin (scientific style).
   */
  svtkSetMacro(DrawAxesAtOrigin, bool);
  svtkGetMacro(DrawAxesAtOrigin, bool);
  svtkBooleanMacro(DrawAxesAtOrigin, bool);
  //@}

  //@{
  /**
   * If true then the axes will be turned on and off depending upon whether
   * any plots are in that corner. Defaults to true.
   */
  svtkSetMacro(AutoAxes, bool);
  svtkGetMacro(AutoAxes, bool);
  svtkBooleanMacro(AutoAxes, bool);
  //@}

  //@{
  /**
   * Border size of the axes that are hidden (svtkAxis::GetVisible())
   */
  svtkSetMacro(HiddenAxisBorder, int);
  svtkGetMacro(HiddenAxisBorder, int);
  //@}

  //@{
  /**
   * Force the axes to have their Minimum and Maximum properties inside the
   * plot boundaries. It constrains pan and zoom interaction.
   * False by default.
   */
  svtkSetMacro(ForceAxesToBounds, bool);
  svtkGetMacro(ForceAxesToBounds, bool);
  svtkBooleanMacro(ForceAxesToBounds, bool);
  //@}

  //@{
  /**
   * Set the width fraction for any bar charts drawn in this chart. It is
   * assumed that all bar plots will use the same array for the X axis, and that
   * this array is regularly spaced. The delta between the first two x values is
   * used to calculated the width of the bars, and subdivided between each bar.
   * The default value is 0.8, 1.0 would lead to bars that touch.
   */
  svtkSetMacro(BarWidthFraction, float);
  svtkGetMacro(BarWidthFraction, float);
  //@}

  //@{
  /**
   * Set the behavior of the mouse wheel.  If true, the mouse wheel zooms in/out
   * on the chart.  Otherwise, unless MouseWheelEvent is overridden by a subclass
   * the mouse wheel does nothing.
   * The default value is true.
   */
  svtkSetMacro(ZoomWithMouseWheel, bool);
  svtkGetMacro(ZoomWithMouseWheel, bool);
  svtkBooleanMacro(ZoomWithMouseWheel, bool);
  //@}

  //@{
  /**
   * Adjust the minimum of a logarithmic axis to be greater than 0, regardless
   * of the minimum data value.
   * False by default.
   */
  svtkSetMacro(AdjustLowerBoundForLogPlot, bool);
  svtkGetMacro(AdjustLowerBoundForLogPlot, bool);
  svtkBooleanMacro(AdjustLowerBoundForLogPlot, bool);
  //@}

  //@{
  /**
   * Set if the point can be dragged along X
   * by the ClickAndDrag Action
   * True by default.
   */
  svtkSetMacro(DragPointAlongX, bool);
  svtkGetMacro(DragPointAlongX, bool);
  svtkBooleanMacro(DragPointAlongX, bool);
  //@}

  //@{
  /**
   * Set if the point can be dragged along Y
   * by the ClickAndDrag Action
   * True by default.
   */
  svtkSetMacro(DragPointAlongY, bool);
  svtkGetMacro(DragPointAlongY, bool);
  svtkBooleanMacro(DragPointAlongY, bool);
  //@}

  /**
   * Set the information passed to the tooltip.
   */
  virtual void SetTooltipInfo(const svtkContextMouseEvent&, const svtkVector2d&, svtkIdType, svtkPlot*,
    svtkIdType segmentIndex = -1);

  /**
   * Return true if the supplied x, y coordinate is inside the item.
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
   * Mouse button down event
   */
  bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   */
  bool MouseButtonReleaseEvent(const svtkContextMouseEvent& mouse) override;

  /**
   * Mouse wheel event, positive delta indicates forward movement of the wheel.
   */
  bool MouseWheelEvent(const svtkContextMouseEvent& mouse, int delta) override;

  /**
   * Key press event.
   */
  bool KeyPressEvent(const svtkContextKeyEvent& key) override;

  /**
   * Populate the annotation link with the supplied selectionIds array, and set
   * the appropriate node properties for a standard row based chart selection.
   */
  static void MakeSelection(svtkAnnotationLink* link, svtkIdTypeArray* selectionIds, svtkPlot* plot);

  /**
   * Subtract the supplied selection from the oldSelection.
   */
  static void MinusSelection(svtkIdTypeArray* selection, svtkIdTypeArray* oldSelection);

  /**
   * Add the supplied selection from the oldSelection.
   */
  static void AddSelection(svtkIdTypeArray* selection, svtkIdTypeArray* oldSelection);

  /**
   * Toggle the supplied selection from the oldSelection.
   */
  static void ToggleSelection(svtkIdTypeArray* selection, svtkIdTypeArray* oldSelection);

  /**
   * Build a selection based on the supplied selectionMode using the new
   * plotSelection and combining it with the oldSelection. If link is not nullptr
   * then the resulting selection will be set on the link.
   */
  static void BuildSelection(svtkAnnotationLink* link, int selectionMode,
    svtkIdTypeArray* plotSelection, svtkIdTypeArray* oldSelection, svtkPlot* plot);

  /**
   * Combine the SelectionMode with any mouse modifiers to get an effective
   * selection mode for this click event.
   */
  static int GetMouseSelectionMode(const svtkContextMouseEvent& mouse, int selectionMode);

protected:
  svtkChartXY();
  ~svtkChartXY() override;

  /**
   * Recalculate the necessary transforms.
   */
  void RecalculatePlotTransforms();

  /**
   * Calculate the optimal zoom level such that all of the points to be plotted
   * will fit into the plot area.
   */
  void RecalculatePlotBounds();

  /**
   * Update the layout of the chart, this may require the svtkContext2D in order
   * to get font metrics etc. Initially this was added to resize the charts
   * according in response to the size of the axes.
   */
  virtual bool UpdateLayout(svtkContext2D* painter);

  /**
   * Layout for the legend if it is visible. This is run after the axes layout
   * and will adjust the borders to account for the legend position.
   * \return The required space in the specified border.
   */
  virtual int GetLegendBorder(svtkContext2D* painter, int axisPosition);

  /**
   * Called after the edges of the chart are decided, set the position of the
   * legend, depends upon its alignment.
   */
  virtual void SetLegendPosition(const svtkRectf& rect);

  /**
   * The legend for the chart.
   */
  svtkSmartPointer<svtkChartLegend> Legend;

  /**
   * The tooltip item for the chart - can be used to display extra information.
   */
  svtkSmartPointer<svtkTooltipItem> Tooltip;

  /**
   * Does the plot area transform need to be recalculated?
   */
  bool PlotTransformValid;

  /**
   * The box created as the mouse is dragged around the screen.
   */
  svtkRectf MouseBox;

  /**
   * Should the box be drawn (could be selection, zoom etc).
   */
  bool DrawBox;

  /**
   * The polygon created as the mouse is dragged around the screen when in
   * polygonal selection mode.
   */
  svtkContextPolygon SelectionPolygon;

  /**
   * Should the selection polygon be drawn.
   */
  bool DrawSelectionPolygon;

  /**
   * Should we draw the location of the nearest point on the plot?
   */
  bool DrawNearestPoint;

  /**
   * Keep the axes drawn at the origin? This will attempt to keep the axes drawn
   * at the origin, i.e. 0.0, 0.0 for the chart. This is often the preferred
   * way of drawing scientific/mathematical charts.
   */
  bool DrawAxesAtOrigin;

  /**
   * Should axes be turned on and off automatically - defaults to on.
   */
  bool AutoAxes;

  /**
   * Size of the border when an axis is hidden
   */
  int HiddenAxisBorder;

  /**
   * The fraction of the interval taken up along the x axis by any bars that are
   * drawn on the chart.
   */
  float BarWidthFraction;

  /**
   * Property to force the axes to have their Minimum and Maximum properties
   * inside the plot boundaries. It constrains pan and zoom interaction.
   * False by default.
   */
  bool ForceAxesToBounds;

  /**
   * Property to enable zooming the chart with the mouse wheel.
   * True by default.
   */
  bool ZoomWithMouseWheel;

  /**
   * Property to adjust the minimum of a logarithmic axis to be greater than 0,
   * regardless of the minimum data value.
   */
  bool AdjustLowerBoundForLogPlot;

  /**
   * Properties to enable the drag of a point for the ClickAndDrag Action
   */
  bool DragPointAlongX;
  bool DragPointAlongY;

private:
  svtkChartXY(const svtkChartXY&) = delete;
  void operator=(const svtkChartXY&) = delete;

  svtkChartXYPrivate* ChartPrivate; // Private class where I hide my STL containers

  /**
   * Internal variable to handle update of drag:
   * true if a point has been selected by the user click.
   */
  bool DragPoint;

  /**
   * Figure out the spacing between the bar chart plots, and their offsets.
   */
  void CalculateBarPlots();

  /**
   * Try to locate a point within the plots to display in a tooltip.
   * If invokeEvent is greater than 0, then an event will be invoked if a point
   * is at that mouse position.
   */
  bool LocatePointInPlots(const svtkContextMouseEvent& mouse, int invokeEvent = -1);

  int LocatePointInPlot(const svtkVector2f& position, const svtkVector2f& tolerance,
    svtkVector2f& plotPos, svtkPlot* plot, svtkIdType& segmentIndex);

  /**
   * Remove the plot from the plot corners list.
   */
  bool RemovePlotFromCorners(svtkPlot* plot);

  void ZoomInAxes(svtkAxis* x, svtkAxis* y, float* orign, float* max);

  /**
   * Remove all the selection from Plots.
   * The method does not call InvokeEvent(svtkCommand::SelectionChangedEvent)
   */
  void ReleasePlotSelections();

  /**
   * Transform the selection box or polygon.
   */
  void TransformBoxOrPolygon(bool polygonMode, svtkTransform2D* transform,
    const svtkVector2f& mousePosition, svtkVector2f& min, svtkVector2f& max,
    svtkContextPolygon& polygon);
};

//@{
/**
 * Small struct used by InvokeEvent to send some information about the point
 * that was clicked on. This is an experimental part of the API, subject to
 * change.
 */
struct svtkChartPlotData
{
  svtkStdString SeriesName;
  svtkVector2f Position;
  svtkVector2i ScreenPosition;
  int Index;
};
//@}

#endif // svtkChartXY_h
