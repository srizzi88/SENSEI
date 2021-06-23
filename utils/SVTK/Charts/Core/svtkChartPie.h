/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkChartPie.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkChartPie
 * @brief   Factory class for drawing pie charts
 *
 *
 * This class implements an pie chart.
 */

#ifndef svtkChartPie_h
#define svtkChartPie_h

#include "svtkChart.h"
#include "svtkChartsCoreModule.h" // For export macro

class svtkChartLegend;
class svtkTooltipItem;
class svtkChartPiePrivate;

class SVTKCHARTSCORE_EXPORT svtkChartPie : public svtkChart
{
public:
  svtkTypeMacro(svtkChartPie, svtkChart);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static svtkChartPie* New();

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
   * Add a plot to the chart.
   */
  svtkPlot* AddPlot(int type) override;

  /**
   * Add a plot to the chart. Return the index of the plot, -1 if it failed.
   */
  svtkIdType AddPlot(svtkPlot* plot) override { return Superclass::AddPlot(plot); }

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  svtkPlot* GetPlot(svtkIdType index) override;

  /**
   * Get the number of plots the chart contains.
   */
  svtkIdType GetNumberOfPlots() override;

  /**
   * Set whether the chart should draw a legend.
   */
  void SetShowLegend(bool visible) override;

  /**
   * Get the legend for the chart, if available. Can return nullptr if there is no
   * legend.
   */
  svtkChartLegend* GetLegend() override;

  /**
   * Set the svtkContextScene for the item, always set for an item in a scene.
   */
  void SetScene(svtkContextScene* scene) override;

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
  svtkChartPie();
  ~svtkChartPie() override;

  /**
   * Recalculate the necessary transforms.
   */
  void RecalculatePlotTransforms();

  /**
   * The legend for the chart.
   */
  svtkChartLegend* Legend;

  /**
   * The tooltip item for the chart - can be used to display extra information.
   */
  svtkTooltipItem* Tooltip;

  /**
   * Does the plot area transform need to be recalculated?
   */
  bool PlotTransformValid;

private:
  svtkChartPie(const svtkChartPie&) = delete;
  void operator=(const svtkChartPie&) = delete;

  /**
   * Try to locate a point within the plots to display in a tooltip
   */
  bool LocatePointInPlots(const svtkContextMouseEvent& mouse);

  /**
   * Private implementation details
   */
  svtkChartPiePrivate* Private;
};

#endif // svtkChartPie_h
