/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScalarsToColorsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkScalarsToColorsItem
 * @brief   Abstract class for ScalarsToColors items.
 *
 * svtkScalarsToColorsItem implements item bounds and painting for inherited
 * classes that provide a texture (ComputeTexture()) and optionally a shape
 * @sa
 * svtkControlPointsItem
 * svtkLookupTableItem
 * svtkColorTransferFunctionItem
 * svtkCompositeTransferFunctionItem
 * svtkPiecewiseItemFunctionItem
 */

#ifndef svtkScalarsToColorsItem_h
#define svtkScalarsToColorsItem_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkNew.h"              // For svtkNew
#include "svtkPlot.h"

class svtkCallbackCommand;
class svtkImageData;
class svtkPlotBar;
class svtkPoints2D;

class SVTKCHARTSCORE_EXPORT svtkScalarsToColorsItem : public svtkPlot
{
public:
  svtkTypeMacro(svtkScalarsToColorsItem, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Bounds of the item, use the UserBounds if valid otherwise compute
   * the bounds of the item (based on the transfer function range).
   */
  void GetBounds(double bounds[4]) override;

  //@{
  /**
   * Set custom bounds, except if bounds are invalid, bounds will be
   * automatically computed based on the range of the control points
   * Invalid bounds by default.
   */
  svtkSetVector4Macro(UserBounds, double);
  svtkGetVector4Macro(UserBounds, double);
  //@}

  /**
   * Paint the texture into a rectangle defined by the bounds. If
   * MaskAboveCurve is true and a shape has been provided by a subclass, it
   * draws the texture into the shape
   */
  bool Paint(svtkContext2D* painter) override;

  //@{
  /**
   * Get a pointer to the svtkPen object that controls the drawing of the edge
   * of the shape if any.
   * PolyLinePen type is svtkPen::NO_PEN by default.
   */
  svtkGetObjectMacro(PolyLinePen, svtkPen);
  //@}

  //@{
  /**
   * Set/Get the svtkTable displayed as an histogram using a svtkPlotBar
   */
  void SetHistogramTable(svtkTable* histogramTable);
  svtkGetObjectMacro(HistogramTable, svtkTable);
  //@}

  //@{
  /**
   * Don't fill in the part above the transfer function.
   * If true texture is not visible above the shape provided by subclasses,
   * otherwise the whole rectangle defined by the bounds is filled with the
   * transfer function.
   * Note: only 2D transfer functions (RGB tf + alpha tf ) support the feature.
   */
  svtkSetMacro(MaskAboveCurve, bool);
  svtkGetMacro(MaskAboveCurve, bool);
  //@}

  /**
   * Function to query a plot for the nearest point to the specified coordinate.
   * Returns the index of the data series with which the point is associated or
   * -1.
   * If a svtkIdType* is passed, its referent will be set to index of the bar
   * segment with which a point is associated, or -1.
   */
  virtual svtkIdType GetNearestPoint(const svtkVector2f& point, const svtkVector2f&,
    svtkVector2f* location, svtkIdType* segmentIndex) override;
#ifndef SVTK_LEGACY_REMOVE
  using svtkPlot::GetNearestPoint;
#endif // SVTK_LEGACY_REMOVE

  /**
   * Generate and return the tooltip label string for this plot
   * The segmentIndex is implemented here.
   */
  svtkStdString GetTooltipLabel(
    const svtkVector2d& plotPos, svtkIdType seriesIndex, svtkIdType segmentIndex) override;

protected:
  svtkScalarsToColorsItem();
  ~svtkScalarsToColorsItem() override;

  /**
   * Bounds of the item, by default (0, 1, 0, 1) but it depends on the
   * range of the ScalarsToColors function.
   * Need to be reimplemented by subclasses if the range is != [0,1]
   */
  virtual void ComputeBounds(double* bounds);

  /**
   * Need to be reimplemented by subclasses, ComputeTexture() is called at
   * paint time if the texture is not up to date compared to svtkScalarsToColorsItem
   * Return false if no texture is generated.
   */
  virtual void ComputeTexture() = 0;

  svtkGetMacro(TextureWidth, int);

  /**
   * Method to configure the plotbar histogram before painting it
   * can be reimplemented by subclasses.
   * Return true if the histogram should be painted, false otherwise.
   */
  virtual bool ConfigurePlotBar();

  //@{
  /**
   * Called whenever the ScalarsToColors function(s) is modified. It internally
   * calls Modified(). Can be reimplemented by subclasses
   */
  virtual void ScalarsToColorsModified(svtkObject* caller, unsigned long eid, void* calldata);
  static void OnScalarsToColorsModified(
    svtkObject* caller, unsigned long eid, void* clientdata, void* calldata);
  //@}

  double UserBounds[4];

  bool Interpolate = true;
  int TextureWidth;
  svtkImageData* Texture = nullptr;
  svtkTable* HistogramTable = nullptr;

  svtkNew<svtkPoints2D> Shape;
  svtkNew<svtkCallbackCommand> Callback;
  svtkNew<svtkPlotBar> PlotBar;
  svtkNew<svtkPen> PolyLinePen;
  bool MaskAboveCurve;

private:
  svtkScalarsToColorsItem(const svtkScalarsToColorsItem&) = delete;
  void operator=(const svtkScalarsToColorsItem&) = delete;
};

#endif
