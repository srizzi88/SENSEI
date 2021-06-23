/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotBag.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotBag
 * @brief   Class for drawing an a bagplot.
 *
 *
 * This class allows to draw a bagplot given three columns from
 * a svtkTable. The first two columns will represent X,Y as it is for
 * svtkPlotPoints. The third one will have to specify if the density
 * assigned to each point (generally obtained by the
 * svtkHighestDensityRegionsStatistics filter).
 * Points are drawn in a plot points fashion and 2 convex hull polygons
 * are drawn around the median and the 3 quartile of the density field.
 *
 * @sa
 * svtkHighestDensityRegionsStatistics
 */

#ifndef svtkPlotBag_h
#define svtkPlotBag_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkPlotPoints.h"

class svtkPen;

class SVTKCHARTSCORE_EXPORT svtkPlotBag : public svtkPlotPoints
{
public:
  svtkTypeMacro(svtkPlotBag, svtkPlotPoints);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a new Bag Plot object.
   */
  static svtkPlotBag* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn.
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
   * Get the plot labels. If this array has a length greater than 1 the index
   * refers to the stacked objects in the plot. See svtkPlotBar for example.
   */
  svtkStringArray* GetLabels() override;

  /**
   * Generate and return the tooltip label string for this plot
   * The segmentIndex parameter is ignored, except for svtkPlotBar
   */
  svtkStdString GetTooltipLabel(
    const svtkVector2d& plotPos, svtkIdType seriesIndex, svtkIdType segmentIndex) override;

  //@{
  /**
   * Set the input, we are expecting a svtkTable with three columns. The first
   * column and the second represent the x,y position . The five others
   * columns represent the quartiles used to display the box.
   * Inherited method will call the last SetInputData method with default
   * parameters.
   */
  void SetInputData(svtkTable* table) override;
  void SetInputData(
    svtkTable* table, const svtkStdString& yColumn, const svtkStdString& densityColumn) override;
  virtual void SetInputData(svtkTable* table, const svtkStdString& xColumn,
    const svtkStdString& yColumn, const svtkStdString& densityColumn);
  //@}

  virtual void SetInputData(
    svtkTable* table, svtkIdType xColumn, svtkIdType yColumn, svtkIdType densityColumn);

  //@{
  /**
   * Set/get the visibility of the bags.
   * True by default.
   */
  svtkSetMacro(BagVisible, bool);
  svtkGetMacro(BagVisible, bool);
  //@}

  //@{
  /**
   * Set/get the svtkPen object that controls how this plot draws boundary lines.
   */
  void SetLinePen(svtkPen* pen);
  svtkGetObjectMacro(LinePen, svtkPen);
  //@}

  /**
   * Set/get the svtkPen object that controls how this plot draws points.
   * Those are just helpers functions:
   * this pen is actually the default Plot pen.
   */
  void SetPointPen(svtkPen* pen) { this->SetPen(pen); }
  svtkPen* GetPointPen() { return this->GetPen(); }

protected:
  svtkPlotBag();
  ~svtkPlotBag() override;

  void UpdateTableCache(svtkDataArray*);

  bool BagVisible;
  svtkPoints2D* MedianPoints;
  svtkPoints2D* Q3Points;
  svtkPen* LinePen;

private:
  svtkPlotBag(const svtkPlotBag&) = delete;
  void operator=(const svtkPlotBag&) = delete;
};

#endif // svtkPlotBag_h
