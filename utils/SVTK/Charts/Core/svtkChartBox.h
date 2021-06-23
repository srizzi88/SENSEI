/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChartBox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkChartBox
 * @brief   Factory class for drawing box plot charts
 *
 *
 * This defines the interface for a box plot chart.
 */

#ifndef svtkChartBox_h
#define svtkChartBox_h

#include "svtkChart.h"
#include "svtkChartsCoreModule.h" // For export macro

class svtkIdTypeArray;
class svtkPlotBox;
class svtkStdString;
class svtkStringArray;
class svtkTooltipItem;

class SVTKCHARTSCORE_EXPORT svtkChartBox : public svtkChart
{
public:
  svtkTypeMacro(svtkChartBox, svtkChart);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a box chart
   */
  static svtkChartBox* New();

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

  //@{
  /**
   * Set the visibility of the specified column.
   */
  void SetColumnVisibility(const svtkStdString& name, bool visible);
  void SetColumnVisibility(svtkIdType column, bool visible);
  //@}

  /**
   * Set the visibility of all columns (true will make them all visible, false
   * will remove all visible columns).
   */
  void SetColumnVisibilityAll(bool visible);

  //@{
  /**
   * Get the visibility of the specified column.
   */
  bool GetColumnVisibility(const svtkStdString& name);
  bool GetColumnVisibility(svtkIdType column);
  //@}

  /**
   * Get the input table column id of a column by its name.
   */
  svtkIdType GetColumnId(const svtkStdString& name);

  //@{
  /**
   * Get a list of the columns, and the order in which they are displayed.
   */
  svtkGetObjectMacro(VisibleColumns, svtkStringArray);
  //@}

  // Index of the selected column in the visible columns list.
  svtkGetMacro(SelectedColumn, int);
  svtkSetMacro(SelectedColumn, int);

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  svtkPlot* GetPlot(svtkIdType index) override;

  /**
   * Get the number of plots the chart contains.
   */
  svtkIdType GetNumberOfPlots() override;

  /**
   * Get the chart Y axis
   */
  virtual svtkAxis* GetYAxis();

  /**
   * Get the column X position by index in the visible set.
   */
  virtual float GetXPosition(int index);

  /**
   * Get the number of visible box plots in the current chart.
   */
  virtual svtkIdType GetNumberOfVisibleColumns();

  /**
   * Set plot to use for the chart. Since this type of chart can
   * only contain one plot, this will replace the previous plot.
   */
  virtual void SetPlot(svtkPlotBox* plot);

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

  /**
   * Set the svtkTooltipItem object that will be displayed by the chart.
   */
  virtual void SetTooltip(svtkTooltipItem* tooltip);

  /**
   * Get the svtkTooltipItem object that will be displayed by the chart.
   */
  virtual svtkTooltipItem* GetTooltip();

  /**
   * Set the information passed to the tooltip.
   */
  virtual void SetTooltipInfo(const svtkContextMouseEvent&, const svtkVector2d&, svtkIdType, svtkPlot*,
    svtkIdType segmentIndex = -1);

protected:
  svtkChartBox();
  ~svtkChartBox() override;

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
   * A list of the visible columns in the chart.
   */
  svtkStringArray* VisibleColumns;

  //@{
  /**
   * Index of the selected column in the visible columns list.
   */
  int SelectedColumn;
  float SelectedColumnDelta;
  //@}

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  svtkTimeStamp BuildTime;

  /**
   * The tooltip item for the chart - can be used to display extra information.
   */
  svtkSmartPointer<svtkTooltipItem> Tooltip;

  void ResetSelection();
  void UpdateGeometry(svtkContext2D*);
  void CalculatePlotTransform();
  void SwapAxes(int a1, int a2);

  /**
   * Try to locate a point within the plots to display in a tooltip.
   * If invokeEvent is greater than 0, then an event will be invoked if a point
   * is at that mouse position.
   */
  bool LocatePointInPlots(const svtkContextMouseEvent& mouse, int invokeEvent = -1);

  int LocatePointInPlot(const svtkVector2f& position, const svtkVector2f& tolerance,
    svtkVector2f& plotPos, svtkPlot* plot, svtkIdType& segmentIndex);

private:
  svtkChartBox(const svtkChartBox&) = delete;
  void operator=(const svtkChartBox&) = delete;
};

//@{
/**
 * Small struct used by InvokeEvent to send some information about the point
 * that was clicked on. This is an experimental part of the API, subject to
 * change.
 */
struct svtkChartBoxData
{
  svtkStdString SeriesName;
  svtkVector2f Position;
  svtkVector2i ScreenPosition;
  int Index;
};
//@}

#endif // svtkChartBox_h
