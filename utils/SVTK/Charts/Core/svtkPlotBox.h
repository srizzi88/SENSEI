/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotBox.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotBox
 * @brief   Class for drawing box plots.
 *
 *
 * Plots to draw box plots given columns from a svtkTable that may contain
 * 5 lines with quartiles and median.
 */

#ifndef svtkPlotBox_h
#define svtkPlotBox_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkPlot.h"
#include "svtkStdString.h" // For svtkStdString ivars

class svtkBrush;
class svtkTextProperty;
class svtkTable;
class svtkStdString;
class svtkScalarsToColors;

class SVTKCHARTSCORE_EXPORT svtkPlotBox : public svtkPlot
{
public:
  svtkTypeMacro(svtkPlotBox, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a box plot.
   */
  static svtkPlotBox* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the plot, called whenever the chart needs to be drawn
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * Paint legend event for the plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied.
   */
  bool PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int legendIndex) override;

  //@{
  /**
   * This is a convenience function to set the input table.
   */
  void SetInputData(svtkTable* table) override;
  void SetInputData(svtkTable* table, const svtkStdString&, const svtkStdString&) override
  {
    this->SetInputData(table);
  }
  //@}

  /**
   * Get the plot labels. If this array has a length greater than 1 the index
   * refers to the stacked objects in the plot.
   */
  svtkStringArray* GetLabels() override;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated
   * or -1.
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

  //@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  void SetLookupTable(svtkScalarsToColors* lut);
  svtkScalarsToColors* GetLookupTable();
  //@}

  /**
   * Helper function to set the color of a given column.
   */
  void SetColumnColor(const svtkStdString& colName, double* rgb);

  /**
   * Create default lookup table. Generally used to create one when none
   * is available with the scalar data.
   */
  virtual void CreateDefaultLookupTable();

  //@{
  /**
   * Get/Set the width of boxes.
   */
  svtkGetMacro(BoxWidth, float);
  svtkSetMacro(BoxWidth, float);
  //@}

  //@{
  /**
   * Get the svtkTextProperty that governs how the plot title is displayed.
   */
  svtkGetObjectMacro(TitleProperties, svtkTextProperty);
  //@}

protected:
  svtkPlotBox();
  ~svtkPlotBox() override;

  void DrawBoxPlot(int, unsigned char*, double, svtkContext2D*);

  /**
   * Update the table cache.
   */
  bool UpdateTableCache(svtkTable* table);

  //@{
  /**
   * Store a well packed set of XY coordinates for this data series.
   */
  class Private;
  Private* Storage;
  //@}

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  svtkTimeStamp BuildTime;

  /**
   * Width of boxes.
   */
  float BoxWidth;

  /**
   * Lookup Table for coloring points by scalar value
   */
  svtkScalarsToColors* LookupTable;

  /**
   * Text properties for the plot title
   */
  svtkTextProperty* TitleProperties;

private:
  svtkPlotBox(const svtkPlotBox&) = delete;
  void operator=(const svtkPlotBox&) = delete;
};

#endif // svtkPlotBox_h
