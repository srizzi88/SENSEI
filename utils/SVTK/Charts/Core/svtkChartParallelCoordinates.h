/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChartParallelCoordinates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkChartParallelCoordinates
 * @brief   Factory class for drawing 2D charts
 *
 *
 * This defines the interface for a parallel coordinates chart.
 */

#ifndef svtkChartParallelCoordinates_h
#define svtkChartParallelCoordinates_h

#include "svtkChart.h"
#include "svtkChartsCoreModule.h" // For export macro
#include "svtkNew.h"              // For svtkNew

class svtkIdTypeArray;
class svtkStdString;
class svtkStringArray;
class svtkPlotParallelCoordinates;

class SVTKCHARTSCORE_EXPORT svtkChartParallelCoordinates : public svtkChart
{
public:
  svtkTypeMacro(svtkChartParallelCoordinates, svtkChart);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a parallel coordinates chart
   */
  static svtkChartParallelCoordinates* New();

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
   * Set the visibility of the specified column.
   */
  void SetColumnVisibility(const svtkStdString& name, bool visible);

  /**
   * Set the visibility of all columns (true will make them all visible, false
   * will remove all visible columns).
   */
  void SetColumnVisibilityAll(bool visible);

  /**
   * Get the visibility of the specified column.
   */
  bool GetColumnVisibility(const svtkStdString& name);

  /**
   * Get a list of the columns, and the order in which they are displayed.
   */
  virtual svtkStringArray* GetVisibleColumns();

  /**
   * Set the list of visible columns, and the order in which they will be displayed.
   */
  virtual void SetVisibleColumns(svtkStringArray* visColumns);

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  svtkPlot* GetPlot(svtkIdType index) override;

  /**
   * Get the number of plots the chart contains.
   */
  svtkIdType GetNumberOfPlots() override;

  /**
   * Get the axis specified by axisIndex.
   */
  svtkAxis* GetAxis(int axisIndex) override;

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
   * Set plot to use for the chart. Since this type of chart can
   * only contain one plot, this will replace the previous plot.
   */
  virtual void SetPlot(svtkPlotParallelCoordinates* plot);

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

protected:
  svtkChartParallelCoordinates();
  ~svtkChartParallelCoordinates() override;

  //@{
  /**
   * Private storage object - where we hide all of our STL objects...
   */
  class Private;
  Private* Storage;
  //@}

  bool GeometryValid;

  /**
   * Selected indices for the table the plot is rendering
   */
  svtkIdTypeArray* Selection;

  /**
   * Strongly owned internal data for the column visibility.
   */
  svtkNew<svtkStringArray> VisibleColumns;

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  svtkTimeStamp BuildTime;

  void ResetSelection();
  bool ResetAxeSelection(int axe);
  void ResetAxesSelection();
  void UpdateGeometry();
  void CalculatePlotTransform();
  void SwapAxes(int a1, int a2);

private:
  svtkChartParallelCoordinates(const svtkChartParallelCoordinates&) = delete;
  void operator=(const svtkChartParallelCoordinates&) = delete;
};

#endif // svtkChartParallelCoordinates_h
