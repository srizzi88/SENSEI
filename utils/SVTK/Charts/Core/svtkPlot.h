/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlot.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlot
 * @brief   Abstract class for 2D plots.
 *
 *
 * The base class for all plot types used in svtkChart derived charts.
 *
 * @sa
 * svtkPlotPoints svtkPlotLine svtkPlotBar svtkChart svtkChartXY
 */

#ifndef svtkPlot_h
#define svtkPlot_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkContextItem.h"
#include "svtkContextPolygon.h" // For svtkContextPolygon
#include "svtkRect.h"           // For svtkRectd ivar
#include "svtkSmartPointer.h"   // Needed to hold SP ivars
#include "svtkStdString.h"      // Needed to hold TooltipLabelFormat ivar

class svtkVariant;
class svtkTable;
class svtkIdTypeArray;
class svtkContextMapper2D;
class svtkPen;
class svtkBrush;
class svtkAxis;
class svtkStringArray;

class SVTKCHARTSCORE_EXPORT svtkPlot : public svtkContextItem
{
public:
  svtkTypeMacro(svtkPlot, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set whether the plot renders an entry in the legend. Default is true.
   * svtkPlot::PaintLegend will get called to render the legend marker on when
   * this is true.
   */
  svtkSetMacro(LegendVisibility, bool);
  svtkGetMacro(LegendVisibility, bool);
  svtkBooleanMacro(LegendVisibility, bool);
  //@}

  /**
   * Paint legend event for the plot, called whenever the legend needs the
   * plot items symbol/mark/line drawn. A rect is supplied with the lower left
   * corner of the rect (elements 0 and 1) and with width x height (elements 2
   * and 3). The plot can choose how to fill the space supplied. The index is used
   * by Plots that return more than one label.
   */
  virtual bool PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int legendIndex);

  //@{
  /**
   * Sets/gets a printf-style string to build custom tooltip labels from.
   * An empty string generates the default tooltip labels.
   * The following case-sensitive format tags (without quotes) are recognized:
   * '%x' The X value of the plot element
   * '%y' The Y value of the plot element
   * '%i' The IndexedLabels entry for the plot element
   * '%l' The value of the plot's GetLabel() function
   * '%s' (svtkPlotBar only) The Labels entry for the bar segment
   * Any other characters or unrecognized format tags are printed in the
   * tooltip label verbatim.
   */
  virtual void SetTooltipLabelFormat(const svtkStdString& label);
  virtual svtkStdString GetTooltipLabelFormat();
  //@}

  //@{
  /**
   * Sets/gets the tooltip notation style.
   */
  virtual void SetTooltipNotation(int notation);
  virtual int GetTooltipNotation();
  //@}

  //@{
  /**
   * Sets/gets the tooltip precision.
   */
  virtual void SetTooltipPrecision(int precision);
  virtual int GetTooltipPrecision();
  //@}

  /**
   * Generate and return the tooltip label string for this plot
   * The segmentIndex parameter is ignored, except for svtkPlotBar
   */
  virtual svtkStdString GetTooltipLabel(
    const svtkVector2d& plotPos, svtkIdType seriesIndex, svtkIdType segmentIndex);

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated, or
   * -1 if no point was found.
   */
  virtual svtkIdType GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tolerance,
    svtkVector2f* location,
#ifndef SVTK_LEGACY_REMOVE
    svtkIdType* segmentId);
#else
    svtkIdType* segmentId = nullptr);
#endif // SVTK_LEGACY_REMOVE

#ifndef SVTK_LEGACY_REMOVE
  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated, or
   * -1 if no point was found.
   * Deprecated method, uses GetNearestPoint(const svtkVector2f& point, const svtkVector2f& tolerance,
   * svtkVector2f* location, svtkIdType* segmentId); instead.
   */
  SVTK_LEGACY(virtual svtkIdType GetNearestPoint(
    const svtkVector2f& point, const svtkVector2f& tolerance, svtkVector2f* location));
#endif // SVTK_LEGACY_REMOVE

  /**
   * Select all points in the specified rectangle.
   */
  virtual bool SelectPoints(const svtkVector2f& min, const svtkVector2f& max);

  /**
   * Select all points in the specified polygon.
   */
  virtual bool SelectPointsInPolygon(const svtkContextPolygon& polygon);

  //@{
  /**
   * Set the plot color
   */
  virtual void SetColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  virtual void SetColor(double r, double g, double b);
  virtual void GetColor(double rgb[3]);
  void GetColor(unsigned char rgb[3]);
  //@}

  /**
   * Set the width of the line.
   */
  virtual void SetWidth(float width);

  /**
   * Get the width of the line.
   */
  virtual float GetWidth();

  //@{
  /**
   * Set/get the svtkPen object that controls how this plot draws (out)lines.
   */
  void SetPen(svtkPen* pen);
  svtkPen* GetPen();
  //@}

  //@{
  /**
   * Set/get the svtkBrush object that controls how this plot fills shapes.
   */
  void SetBrush(svtkBrush* brush);
  svtkBrush* GetBrush();
  //@}

  //@{
  /**
   * Set/get the svtkBrush object that controls how this plot fills selected
   * shapes.
   */
  void SetSelectionPen(svtkPen* pen);
  svtkPen* GetSelectionPen();
  //@}

  //@{
  /**
   * Set/get the svtkBrush object that controls how this plot fills selected
   * shapes.
   */
  void SetSelectionBrush(svtkBrush* brush);
  svtkBrush* GetSelectionBrush();
  //@}

  /**
   * Set the label of this plot.
   */
  virtual void SetLabel(const svtkStdString& label);

  /**
   * Get the label of this plot.
   */
  virtual svtkStdString GetLabel();

  /**
   * Set the plot labels, these are used for stacked chart variants, with the
   * index referring to the stacking index.
   */
  virtual void SetLabels(svtkStringArray* labels);

  /**
   * Get the plot labels. If this array has a length greater than 1 the index
   * refers to the stacked objects in the plot. See svtkPlotBar for example.
   */
  virtual svtkStringArray* GetLabels();

  /**
   * Get the number of labels associated with this plot.
   */
  virtual int GetNumberOfLabels();

  /**
   * Get the label at the specified index.
   */
  svtkStdString GetLabel(svtkIdType index);

  /**
   * Set indexed labels for the plot. If set, this array can be used to provide
   * custom labels for each point in a plot. This array should be the same
   * length as the points array. Default is null (no indexed labels).
   */
  void SetIndexedLabels(svtkStringArray* labels);

  /**
   * Get the indexed labels array.
   */
  virtual svtkStringArray* GetIndexedLabels();

  /**
   * Get the data object that the plot will draw.
   */
  svtkContextMapper2D* GetData();

  //@{
  /**
   * Use the Y array index for the X value. If true any X column setting will be
   * ignored, and the X values will simply be the index of the Y column.
   */
  svtkGetMacro(UseIndexForXSeries, bool);
  //@}

  //@{
  /**
   * Use the Y array index for the X value. If true any X column setting will be
   * ignored, and the X values will simply be the index of the Y column.
   */
  svtkSetMacro(UseIndexForXSeries, bool);
  //@}

  //@{
  /**
   * This is a convenience function to set the input table and the x, y column
   * for the plot.
   */
  virtual void SetInputData(svtkTable* table);
  virtual void SetInputData(
    svtkTable* table, const svtkStdString& xColumn, const svtkStdString& yColumn);
  void SetInputData(svtkTable* table, svtkIdType xColumn, svtkIdType yColumn);
  //@}

  /**
   * Get the input table used by the plot.
   */
  virtual svtkTable* GetInput();

  /**
   * Convenience function to set the input arrays. For most plots index 0
   * is the x axis, and index 1 is the y axis. The name is the name of the
   * column in the svtkTable.
   */
  virtual void SetInputArray(int index, const svtkStdString& name);

  //@{
  /**
   * Set whether the plot can be selected. True by default.
   * If not, then SetSelection(), SelectPoints() or SelectPointsInPolygon()
   * won't have any effect.
   * \sa SetSelection(), SelectPoints(), SelectPointsInPolygon()
   */
  svtkSetMacro(Selectable, bool);
  svtkGetMacro(Selectable, bool);
  svtkBooleanMacro(Selectable, bool);
  //@}

  //@{
  /**
   * Sets the list of points that must be selected.
   * If Selectable is false, then this method does nothing.
   * \sa SetSelectable()
   */
  virtual void SetSelection(svtkIdTypeArray* id);
  svtkGetObjectMacro(Selection, svtkIdTypeArray);
  //@}

  //@{
  /**
   * Get/set the X axis associated with this plot.
   */
  svtkGetObjectMacro(XAxis, svtkAxis);
  virtual void SetXAxis(svtkAxis* axis);
  //@}

  //@{
  /**
   * Get/set the Y axis associated with this plot.
   */
  svtkGetObjectMacro(YAxis, svtkAxis);
  virtual void SetYAxis(svtkAxis* axis);
  //@}

  //@{
  /**
   * Get/set the origin shift and scaling factor used by the plot, this is
   * normally 0.0 offset and 1.0 scaling, but can be used to render data outside
   * of the single precision range. The chart that owns the plot should set this
   * and ensure the appropriate matrix is used when rendering the plot.
   */
  void SetShiftScale(const svtkRectd& scaling);
  svtkRectd GetShiftScale();
  //@}

  /**
   * Get the bounds for this plot as (Xmin, Xmax, Ymin, Ymax).

   * See \a GetUnscaledInputBounds for more information.
   */
  virtual void GetBounds(double bounds[4]) { bounds[0] = bounds[1] = bounds[2] = bounds[3] = 0.0; }

  /**
   * Provide un-log-scaled bounds for the plot inputs.

   * This function is analogous to GetBounds() with 2 exceptions:
   * 1. It will never return log-scaled bounds even when the
   * x- and/or y-axes are log-scaled.
   * 2. It will always return the bounds along the *input* axes
   * rather than the output chart coordinates. Thus GetXAxis()
   * returns the axis associated with the first 2 bounds entries
   * and GetYAxis() returns the axis associated with the next 2
   * bounds entries.

   * For example, svtkPlotBar's GetBounds() method
   * will swap axis bounds when its orientation is vertical while
   * its GetUnscaledInputBounds() will not swap axis bounds.

   * This method is provided so user interfaces can determine
   * whether or not to allow log-scaling of a particular svtkAxis.

   * Subclasses of svtkPlot are responsible for implementing this
   * function to transform input plot data.

   * The returned \a bounds are stored as (Xmin, Xmax, Ymin, Ymax).
   */
  virtual void GetUnscaledInputBounds(double bounds[4])
  {
    // Implemented here by calling GetBounds() to support plot
    // subclasses that do no log-scaling or plot orientation.
    return this->GetBounds(bounds);
  }

  /**
   * Subclasses that build data caches to speed up painting should override this
   * method to update such caches. This is called on each Paint, hence
   * subclasses must add checks to avoid rebuilding of cache, unless necessary.
   * Default implementation is empty.
   */
  virtual void UpdateCache() {}

  //@{
  /**
   * A General setter/getter that should be overridden. It can silently drop
   * options, case is important
   */
  virtual void SetProperty(const svtkStdString& property, const svtkVariant& var);
  virtual svtkVariant GetProperty(const svtkStdString& property);
  //@}

  //@{
  /**
   * Clamp the given 2D pos into the provided bounds
   * Return true if the pos has been clamped, false otherwise.
   */
  static bool ClampPos(double pos[2], double bounds[4]);
  virtual bool ClampPos(double pos[2]);
  //@}

protected:
  svtkPlot();
  ~svtkPlot() override;

  /**
   * Get the properly formatted number for the supplied position and axis.
   */
  svtkStdString GetNumber(double position, svtkAxis* axis);

  //@{
  /**
   * Transform the mouse event in the control-points space. This is needed when
   * using logScale or shiftscale.
   */
  virtual void TransformScreenToData(const svtkVector2f& in, svtkVector2f& out);
  virtual void TransformDataToScreen(const svtkVector2f& in, svtkVector2f& out);
  virtual void TransformScreenToData(
    const double inX, const double inY, double& outX, double& outY);
  virtual void TransformDataToScreen(
    const double inX, const double inY, double& outX, double& outY);
  //@}

  /**
   * This object stores the svtkPen that controls how the plot is drawn.
   */
  svtkSmartPointer<svtkPen> Pen;

  /**
   * This object stores the svtkBrush that controls how the plot is drawn.
   */
  svtkSmartPointer<svtkBrush> Brush;

  /**
   * This object stores the svtkPen that controls how the selected elements
   * of the plot are drawn.
   */
  svtkSmartPointer<svtkPen> SelectionPen;

  /**
   * This object stores the svtkBrush that controls how the selected elements
   * of the plot are drawn.
   */
  svtkSmartPointer<svtkBrush> SelectionBrush;

  /**
   * Plot labels, used by legend.
   */
  svtkSmartPointer<svtkStringArray> Labels;

  /**
   * Holds Labels when they're auto-created
   */
  svtkSmartPointer<svtkStringArray> AutoLabels;

  /**
   * Holds Labels when they're auto-created
   */
  svtkSmartPointer<svtkStringArray> IndexedLabels;

  /**
   * Use the Y array index for the X value. If true any X column setting will be
   * ignored, and the X values will simply be the index of the Y column.
   */
  bool UseIndexForXSeries;

  /**
   * This data member contains the data that will be plotted, it inherits
   * from svtkAlgorithm.
   */
  svtkSmartPointer<svtkContextMapper2D> Data;

  /**
   * Whether plot points can be selected or not.
   */
  bool Selectable;

  /**
   * Selected indices for the table the plot is rendering
   */
  svtkIdTypeArray* Selection;

  /**
   * The X axis associated with this plot.
   */
  svtkAxis* XAxis;

  /**
   * The X axis associated with this plot.
   */
  svtkAxis* YAxis;

  /**
   * A printf-style string to build custom tooltip labels from.
   * See the accessor/mutator functions for full documentation.
   */
  svtkStdString TooltipLabelFormat;

  /**
   * The default printf-style string to build custom tooltip labels from.
   * See the accessor/mutator functions for full documentation.
   */
  svtkStdString TooltipDefaultLabelFormat;

  int TooltipNotation;
  int TooltipPrecision;

  /**
   * The current shift in origin and scaling factor applied to the plot.
   */
  svtkRectd ShiftScale;

  bool LegendVisibility;

#ifndef SVTK_LEGACY_REMOVE
  /**
   * Flag used by GetNearestPoint legacy implementation
   * to avoid infinite call
   */
  bool LegacyRecursionFlag = false;
#endif // SVTK_LEGACY_REMOVE

private:
  svtkPlot(const svtkPlot&) = delete;
  void operator=(const svtkPlot&) = delete;
};

#endif // svtkPlot_h
