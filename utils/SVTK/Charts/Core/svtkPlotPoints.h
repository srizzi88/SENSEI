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
 * @class   svtkPlotPoints
 * @brief   Class for drawing an points given two columns from a
 * svtkTable.
 *
 *
 * This class draws points in a plot given two columns from a table. If you need
 * a line as well you should use svtkPlotLine which derives from svtkPlotPoints
 * and is capable of drawing both points and a line.
 *
 * @sa
 * svtkPlotLine
 */

#ifndef svtkPlotPoints_h
#define svtkPlotPoints_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkNew.h"              // For ivars
#include "svtkPlot.h"
#include "svtkRenderingCoreEnums.h" // For marker enum
#include "svtkScalarsToColors.h"    // For SVTK_COLOR_MODE_DEFAULT and _MAP_SCALARS
#include "svtkStdString.h"          // For color array name

class svtkCharArray;
class svtkContext2D;
class svtkTable;
class svtkPoints2D;
class svtkFloatArray;
class svtkStdString;
class svtkImageData;
class svtkScalarsToColors;
class svtkUnsignedCharArray;

class SVTKCHARTSCORE_EXPORT svtkPlotPoints : public svtkPlot
{
public:
  svtkTypeMacro(svtkPlotPoints, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static svtkPlotPoints* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Paint legend event for the XY plot, called whenever the legend needs the
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
   * Get the non-log-scaled bounds on chart inputs for this plot as (Xmin, Xmax, Ymin, Ymax).
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

  //@{
  /**
   * Turn on/off flag to control whether scalar data is used to color objects.
   */
  svtkSetMacro(ScalarVisibility, svtkTypeBool);
  svtkGetMacro(ScalarVisibility, svtkTypeBool);
  svtkBooleanMacro(ScalarVisibility, svtkTypeBool);
  //@}

  //@{
  /**
   * When ScalarMode is set to UsePointFieldData or UseCellFieldData,
   * you can specify which array to use for coloring using these methods.
   * The lookup table will decide how to convert vectors to colors.
   */
  void SelectColorArray(svtkIdType arrayNum);
  void SelectColorArray(const svtkStdString& arrayName);
  //@}

  /**
   * Get the array name to color by.
   */
  svtkStdString GetColorArrayName();

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

  /**
   * Enum containing various marker styles that can be used in a plot.
   */
  enum
  {
    NONE = SVTK_MARKER_NONE,
    CROSS = SVTK_MARKER_CROSS,
    PLUS = SVTK_MARKER_PLUS,
    SQUARE = SVTK_MARKER_SQUARE,
    CIRCLE = SVTK_MARKER_CIRCLE,
    DIAMOND = SVTK_MARKER_DIAMOND
  };

  //@{
  /**
   * Get/set the marker style that should be used. The default is none, the enum
   * in this class is used as a parameter.
   */
  svtkGetMacro(MarkerStyle, int);
  svtkSetMacro(MarkerStyle, int);
  //@}

  //@{
  /**
   * Get/set the marker size that should be used. The default is negative, and
   * in that case it is 2.3 times the pen width, if less than 8 will be used.
   */
  svtkGetMacro(MarkerSize, float);
  svtkSetMacro(MarkerSize, float);
  //@}

  //@{
  /**
   * Get/set the valid point mask array name.
   */
  svtkGetMacro(ValidPointMaskName, svtkStdString);
  svtkSetMacro(ValidPointMaskName, svtkStdString);
  //@}

protected:
  svtkPlotPoints();
  ~svtkPlotPoints() override;

  /**
   * Populate the data arrays ready to operate on input data.
   */
  bool GetDataArrays(svtkTable* table, svtkDataArray* array[2]);

  /**
   * Update the table cache.
   */
  bool UpdateTableCache(svtkTable* table);

  /**
   * Calculate the unscaled input bounds from the input arrays.
   */
  void CalculateUnscaledInputBounds();

  /**
   * Handle calculating the log of the x or y series if necessary. Should be
   * called by UpdateTableCache once the data has been updated in Points.
   */
  void CalculateLogSeries();

  /**
   * Find all of the "bad points" in the series. This is mainly used to cache
   * bad points for performance reasons, but could also be used plot the bad
   * points in the future.
   */
  void FindBadPoints();

  /**
   * Calculate the bounds of the plot, ignoring the bad points.
   */
  void CalculateBounds(double bounds[4]);

  /**
   * Create the sorted point list if necessary.
   */
  void CreateSortedPoints();

  //@{
  /**
   * Store a well packed set of XY coordinates for this data series.
   */
  svtkPoints2D* Points;
  svtkNew<svtkFloatArray> SelectedPoints;
  //@}

  //@{
  /**
   * Sorted points, used when searching for the nearest point.
   */
  class VectorPIMPL;
  VectorPIMPL* Sorted;
  //@}

  /**
   * An array containing the indices of all the "bad points", meaning any x, y
   * pair that has an infinity, -infinity or not a number value.
   */
  svtkIdTypeArray* BadPoints;

  /**
   * Array which marks valid points in the array. If nullptr (the default), all
   * points in the input array are considered valid.
   */
  svtkCharArray* ValidPointMask;

  /**
   * Name of the valid point mask array.
   */
  svtkStdString ValidPointMaskName;

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  svtkTimeStamp BuildTime;

  //@{
  /**
   * The marker style that should be used
   */
  int MarkerStyle;
  float MarkerSize;
  //@}

  bool LogX, LogY;

  //@{
  /**
   * Lookup Table for coloring points by scalar value
   */
  svtkScalarsToColors* LookupTable;
  svtkUnsignedCharArray* Colors;
  svtkTypeBool ScalarVisibility;
  svtkStdString ColorArrayName;
  //@}

  /**
   * Cached bounds on the plot input axes
   */
  double UnscaledInputBounds[4];

private:
  svtkPlotPoints(const svtkPlotPoints&) = delete;
  void operator=(const svtkPlotPoints&) = delete;
};

#endif // svtkPlotPoints_h
