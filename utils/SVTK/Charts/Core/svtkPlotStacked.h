/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotStacked
 * @brief   Class for drawing an stacked polygon plot
 * given an X, Ybase, Yextent  in a svtkTable.
 *
 *
 *
 */

#ifndef svtkPlotStacked_h
#define svtkPlotStacked_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkPlot.h"

class svtkChartXY;
class svtkContext2D;
class svtkTable;
class svtkPoints2D;
class svtkStdString;
class svtkImageData;
class svtkColorSeries;

class svtkPlotStackedPrivate;

class SVTKCHARTSCORE_EXPORT svtkPlotStacked : public svtkPlot
{
public:
  svtkTypeMacro(svtkPlotStacked, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a Stacked Plot Object
   */
  static svtkPlotStacked* New();

  //@{
  /**
   * Set the plot color
   */
  void SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) override;
  void SetColor(double r, double g, double b) override;
  void GetColor(double rgb[3]) override;
  //@}

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the Stacked plot, called whenever the chart needs to be drawn
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Paint legend event for the Stacked plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied.
   */
  bool PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int legendIndex) override;

  /**
   * Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Get the unscaled input bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
   * See svtkPlot for more information.
   */
  void GetUnscaledInputBounds(double bounds[4]) override;

  /**
   * When used to set additional arrays, stacked bars are created.
   */
  void SetInputArray(int index, const svtkStdString& name) override;

  /**
   * Set the color series to use if this becomes a stacked bar plot.
   */
  void SetColorSeries(svtkColorSeries* colorSeries);

  /**
   * Get the color series used if when this is a stacked bar plot.
   */
  svtkColorSeries* GetColorSeries();

  /**
   * Get the plot labels.
   */
  svtkStringArray* GetLabels() override;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated or
   * -1.
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
   * Select all points in the specified rectangle.
   */
  bool SelectPoints(const svtkVector2f& min, const svtkVector2f& max) override;

protected:
  svtkPlotStacked();
  ~svtkPlotStacked() override;

  /**
   * Update the table cache.
   */
  bool UpdateTableCache(svtkTable* table);

  // Descript:
  // For stacked plots the Extent data must be greater than (or equal to) the
  // base data. Insure that this is true
  void FixExtent();

  /**
   * Handle calculating the log of the x or y series if necessary. Should be
   * called by UpdateTableCache once the data has been updated in Points.
   */
  void CalculateLogSeries();

  /**
   * An array containing the indices of all the "bad base points", meaning any x, y
   * pair that has an infinity, -infinity or not a number value.
   */
  svtkIdTypeArray* BaseBadPoints;

  /**
   * An array containing the indices of all the "bad extent points", meaning any x, y
   * pair that has an infinity, -infinity or not a number value.
   */
  svtkIdTypeArray* ExtentBadPoints;

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  svtkTimeStamp BuildTime;

  bool LogX, LogY;

  /**
   * The color series to use for each series.
   */
  svtkSmartPointer<svtkColorSeries> ColorSeries;

private:
  svtkPlotStacked(const svtkPlotStacked&) = delete;
  void operator=(const svtkPlotStacked&) = delete;

  svtkPlotStackedPrivate* Private;
};

#endif // svtkPlotStacked_h
