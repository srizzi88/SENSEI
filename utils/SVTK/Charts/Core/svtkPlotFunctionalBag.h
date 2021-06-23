/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotFunctionalBag.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotFunctionalBag
 * @brief   Class for drawing an XY line plot or bag
 * given two columns from a svtkTable.
 *
 *
 * Depending on the number of components, this class will draw either
 * a line plot (for 1 component column) or, for two components columns,
 * a filled polygonal band (the bag) going from the first to the second
 * component on the Y-axis along the X-axis. The filter
 * svtkExtractFunctionalBagPlot is intended to create such "bag" columns.
 *
 * @sa
 * svtkExtractFunctionalBagPlot
 */

#ifndef svtkPlotFunctionalBag_h
#define svtkPlotFunctionalBag_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkNew.h"              // Needed to hold SP ivars
#include "svtkPlot.h"

class svtkDataArray;
class svtkPlotFuntionalBagInternal;
class svtkPlotLine;
class svtkPoints2D;
class svtkScalarsToColors;

class SVTKCHARTSCORE_EXPORT svtkPlotFunctionalBag : public svtkPlot
{
public:
  svtkTypeMacro(svtkPlotFunctionalBag, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a functional bag plot object.
   */
  static svtkPlotFunctionalBag* New();

  /**
   * Returns true if the plot is a functional bag, false if it is a simple
   * line.
   */
  virtual bool IsBag();

  /**
   * Reimplemented to enforce visibility when selected.
   */
  bool GetVisible() override;

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the plot, called whenever the chart needs to be drawn.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Paint legend event for the plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied.
   */
  bool PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int legendIndex) override;

  /**
   * Get the bounds for this plot as (Xmin, Xmax, Ymin, Ymax).
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Get the non-log-scaled bounds on chart inputs for this plot as
   * (Xmin, Xmax, Ymin, Ymax).
   */
  void GetUnscaledInputBounds(double bounds[4]) override;

  //@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  void SetLookupTable(svtkScalarsToColors* lut);
  svtkScalarsToColors* GetLookupTable();
  //@}

  /**
   * Create default lookup table. Generally used to create one when none
   * is available with the scalar data.
   */
  virtual void CreateDefaultLookupTable();

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

  /**
   * Select all points in the specified polygon.
   */
  bool SelectPointsInPolygon(const svtkContextPolygon& polygon) override;

protected:
  svtkPlotFunctionalBag();
  ~svtkPlotFunctionalBag() override;

  /**
   * Populate the data arrays ready to operate on input data.
   */
  bool GetDataArrays(svtkTable* table, svtkDataArray* array[2]);

  /**
   * Update the table cache.
   */
  bool UpdateTableCache(svtkTable*);

  /**
   * The cache is marked dirty until it has been initialized.
   */
  svtkTimeStamp BuildTime;

  /**
   * Lookup Table for coloring points by scalar value
   */
  svtkScalarsToColors* LookupTable;

  /**
   * The plot line delegate for line series
   */
  svtkNew<svtkPlotLine> Line;

  /**
   * The bag points ordered in quadstrip fashion
   */
  svtkNew<svtkPoints2D> BagPoints;

  bool LogX, LogY;

private:
  svtkPlotFunctionalBag(const svtkPlotFunctionalBag&) = delete;
  void operator=(const svtkPlotFunctionalBag&) = delete;
};

#endif // svtkPlotFunctionalBag_h
