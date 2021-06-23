/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotArea.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPlotArea
 * @brief   draws an area plot.
 *
 * svtkPlotArea is used to render an area plot. An area plot (sometimes called a
 * range plot) renders a filled region between the selected ymin and ymax
 * arrays.
 * To specify the x array and ymin/ymax arrays, use the SetInputArray method
 * with array index as 0, 1, or 2, respectively.
 */

#ifndef svtkPlotArea_h
#define svtkPlotArea_h

#include "svtkPlot.h"

class SVTKCHARTSCORE_EXPORT svtkPlotArea : public svtkPlot
{
public:
  static svtkPlotArea* New();
  svtkTypeMacro(svtkPlotArea, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Convenience method to set the input arrays. svtkPlotArea supports the
   * following indices:
   * \li 0: x-axis,
   * \li 1: y-axis,
   * \li 2: y-axis.
   */
  using Superclass::SetInputArray;

  //@{
  /**
   * Overridden to set the brush color.
   */
  void SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) override;
  void SetColor(double r, double g, double b) override;
  //@}

  //@{
  /**
   * Get/set the valid point mask array name.
   */
  svtkGetMacro(ValidPointMaskName, svtkStdString);
  svtkSetMacro(ValidPointMaskName, svtkStdString);
  //@}

  /**
   * Perform any updates to the item that may be necessary before rendering.
   */
  void Update() override;

  /**
   * Get the bounds for this plot as (Xmin, Xmax, Ymin, Ymax).
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Subclasses that build data caches to speed up painting should override this
   * method to update such caches. This is called on each Paint, hence
   * subclasses must add checks to avoid rebuilding of cache, unless necessary.
   */
  void UpdateCache() override;

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Paint legend event for the plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied. The index is used
   * by Plots that return more than one label.
   */
  bool PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int legendIndex) override;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated, or
   * -1 if no point was found.
   */
  svtkIdType GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tolerance,
    svtkVector2f* location,
#ifndef SVTK_LEGACY_REMOVE
    svtkIdType* segmentId) override;
#else
    svtkIdType* segmentId = nullptr) override;
#endif // SVTK_LEGACY_REMOVE

#ifndef SVTK_LEGACY_REMOVE
  using svtkPlot::GetNearestPoint;
#endif // SVTK_LEGACY_REMOVE

  /**
   * Generate and return the tooltip label string for this plot
   * The segmentIndex parameter is ignored, except for svtkPlotBar
   */
  svtkStdString GetTooltipLabel(
    const svtkVector2d& plotPos, svtkIdType seriesIndex, svtkIdType segmentIndex) override;

protected:
  svtkPlotArea();
  ~svtkPlotArea() override;

  /**
   * Name of the valid point mask array.
   */
  svtkStdString ValidPointMaskName;

private:
  svtkPlotArea(const svtkPlotArea&) = delete;
  void operator=(const svtkPlotArea&) = delete;

  class svtkTableCache;
  svtkTableCache* TableCache;

  svtkTimeStamp UpdateTime;
};

#endif
