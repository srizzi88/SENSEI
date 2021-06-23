/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotParallelCoordinates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotParallelCoordinates
 * @brief   Class for drawing a parallel coordinate
 * plot given columns from a svtkTable.
 *
 *
 *
 */

#ifndef svtkPlotParallelCoordinates_h
#define svtkPlotParallelCoordinates_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkPlot.h"
#include "svtkScalarsToColors.h" // For SVTK_COLOR_MODE_DEFAULT and _MAP_SCALARS
#include "svtkStdString.h"       // For svtkStdString ivars

class svtkChartParallelCoordinates;
class svtkTable;
class svtkStdString;
class svtkScalarsToColors;
class svtkUnsignedCharArray;

class SVTKCHARTSCORE_EXPORT svtkPlotParallelCoordinates : public svtkPlot
{
public:
  svtkTypeMacro(svtkPlotParallelCoordinates, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a parallel coordinates chart
   */
  static svtkPlotParallelCoordinates* New();

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
   * Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Set the selection criteria on the given axis in normalized space (0.0 - 1.0).
   */
  bool SetSelectionRange(int Axis, float low, float high);

  /**
   * Reset the selection criteria for the chart.
   */
  bool ResetSelectionRange();

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

protected:
  svtkPlotParallelCoordinates();
  ~svtkPlotParallelCoordinates() override;

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

  //@{
  /**
   * Lookup Table for coloring points by scalar value
   */
  svtkScalarsToColors* LookupTable;
  svtkUnsignedCharArray* Colors;
  svtkTypeBool ScalarVisibility;
  svtkStdString ColorArrayName;
  //@}

private:
  svtkPlotParallelCoordinates(const svtkPlotParallelCoordinates&) = delete;
  void operator=(const svtkPlotParallelCoordinates&) = delete;
};

#endif // svtkPlotParallelCoordinates_h
