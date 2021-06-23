/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotBar.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotBar
 * @brief   Class for drawing an XY plot given two columns from a
 * svtkTable.
 *
 *
 *
 */

#ifndef svtkPlotBar_h
#define svtkPlotBar_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkPlot.h"
#include "svtkSmartPointer.h" // Needed to hold ColorSeries

class svtkContext2D;
class svtkTable;
class svtkPoints2D;
class svtkStdString;
class svtkColorSeries;
class svtkUnsignedCharArray;
class svtkScalarsToColors;

class svtkPlotBarPrivate;

class SVTKCHARTSCORE_EXPORT svtkPlotBar : public svtkPlot
{
public:
  svtkTypeMacro(svtkPlotBar, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Enum of bar chart oritentation types
   */
  enum
  {
    VERTICAL = 0,
    HORIZONTAL
  };

  /**
   * Creates a 2D Chart object.
   */
  static svtkPlotBar* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
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

  //@{
  /**
   * Set the plot color
   */
  void SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) override;
  void SetColor(double r, double g, double b) override;
  void GetColor(double rgb[3]) override;
  //@}

  //@{
  /**
   * Set the width of the line.
   */
  void SetWidth(float _arg) override
  {
    svtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting Width to " << _arg);
    if (this->Width != _arg)
    {
      this->Width = _arg;
      this->Modified();
    }
  }
  //@}

  //@{
  /**
   * Get the width of the line.
   */
  float GetWidth() override
  {
    svtkDebugMacro(<< this->GetClassName() << " (" << this << "): returning Width of "
                  << this->Width);
    return this->Width;
  }
  //@}

  //@{
  /**
   * Set/get the horizontal offset of the bars.
   * Positive values move the bars leftward.
   * For HORIZONTAL orientation, offsets bars vertically,
   * with a positive value moving bars downward.
   */
  svtkSetMacro(Offset, float);
  svtkGetMacro(Offset, float);
  //@}

  //@{
  /**
   * Set/get the orientation of the bars.
   * Valid orientations are VERTICAL (default) and HORIZONTAL.
   */
  virtual void SetOrientation(int orientation);
  svtkGetMacro(Orientation, int);
  //@}

  /**
   * A helper used by both GetUnscaledBounds and GetBounds(double[4]).
   */
  virtual void GetBounds(double bounds[4], bool unscaled);

  /**
   * Get the bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
   */
  void GetBounds(double bounds[4]) override;

  /**
   * Get un-log-scaled bounds for this mapper as (Xmin,Xmax,Ymin,Ymax).
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

  //@{
  /**
   * Specify a lookup table for the mapper to use.
   */
  virtual void SetLookupTable(svtkScalarsToColors* lut);
  virtual svtkScalarsToColors* GetLookupTable();
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
  svtkSetMacro(ScalarVisibility, bool);
  svtkGetMacro(ScalarVisibility, bool);
  svtkBooleanMacro(ScalarVisibility, bool);
  //@}

  //@{
  /**
   * Enable/disable mapping of the opacity values. Default is set to true.
   */
  svtkSetMacro(EnableOpacityMapping, bool);
  svtkGetMacro(EnableOpacityMapping, bool);
  svtkBooleanMacro(EnableOpacityMapping, bool);
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
   * Get the plot labels.
   */
  svtkStringArray* GetLabels() override;

  /**
   * Set the group name of the bar chart - can be displayed on the X axis.
   */
  virtual void SetGroupName(const svtkStdString& name);

  /**
   * Get the group name of the bar char - can be displayed on the X axis.
   */
  virtual svtkStdString GetGroupName();

  /**
   * Generate and return the tooltip label string for this plot
   * The segmentIndex is implemented here.
   */
  svtkStdString GetTooltipLabel(
    const svtkVector2d& plotPos, svtkIdType seriesIndex, svtkIdType segmentIndex) override;

  /**
   * Select all points in the specified rectangle.
   */
  bool SelectPoints(const svtkVector2f& min, const svtkVector2f& max) override;

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated or
   * -1.
   * If a svtkIdType* is passed, its referent will be set to index of the bar
   * segment with which a point is associated, or -1.
   */
  virtual svtkIdType GetNearestPoint(const svtkVector2f& point, const svtkVector2f&,
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
   * Get amount of plotted bars.
   */
  int GetBarsCount();

  /**
   * Get the data bounds for this mapper as (Xmin,Xmax).
   */
  void GetDataBounds(double bounds[2]);

protected:
  svtkPlotBar();
  ~svtkPlotBar() override;

  /**
   * Update the table cache.
   */
  bool UpdateTableCache(svtkTable* table);

  /**
   * Store a well packed set of XY coordinates for this data series.
   */
  svtkPoints2D* Points;

  float Width;
  float Offset;

  int Orientation;

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  svtkTimeStamp BuildTime;

  /**
   * The color series to use if this becomes a stacked bar
   */
  svtkSmartPointer<svtkColorSeries> ColorSeries;

  //@{
  /**
   * Lookup Table for coloring bars by scalar value
   */
  svtkSmartPointer<svtkScalarsToColors> LookupTable;
  svtkSmartPointer<svtkUnsignedCharArray> Colors;
  bool ScalarVisibility;
  bool EnableOpacityMapping;
  svtkStdString ColorArrayName;
  //@}

  bool LogX;
  bool LogY;

private:
  svtkPlotBar(const svtkPlotBar&) = delete;
  void operator=(const svtkPlotBar&) = delete;

  svtkPlotBarPrivate* Private;
};

#endif // svtkPlotBar_h
