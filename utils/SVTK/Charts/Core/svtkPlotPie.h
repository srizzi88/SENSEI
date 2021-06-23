/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotPie.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotPie
 * @brief   Class for drawing a Pie diagram.
 *
 *
 */

#ifndef svtkPlotPie_h
#define svtkPlotPie_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkPlot.h"
#include "svtkSmartPointer.h" // To hold ColorSeries etc.

class svtkContext2D;
class svtkColorSeries;
class svtkPoints2D;

class svtkPlotPiePrivate;

class SVTKCHARTSCORE_EXPORT svtkPlotPie : public svtkPlot
{
public:
  svtkTypeMacro(svtkPlotPie, svtkPlot);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPlotPie* New();

  /**
   * Paint event for the item.
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
   * Set the dimensions of the pie, arguments 1 and 2 are the x and y coordinate
   * of the bottom corner. Arguments 3 and 4 are the width and height.
   */
  void SetDimensions(int arg1, int arg2, int arg3, int arg4);

  /**
   * Set the dimensions of the pie, elements 0 and 1 are the x and y coordinate
   * of the bottom corner. Elements 2 and 3 are the width and height.
   */
  void SetDimensions(const int arg[4]);

  //@{
  /**
   * Get the dimensions of the pie, elements 0 and 1 are the x and y coordinate
   * of the bottom corner. Elements 2 and 3 are the width and height.
   */
  svtkGetVector4Macro(Dimensions, int);
  //@}

  /**
   * Set the color series to use for the Pie.
   */
  void SetColorSeries(svtkColorSeries* colorSeries);

  /**
   * Get the color series used.
   */
  svtkColorSeries* GetColorSeries();

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

protected:
  svtkPlotPie();
  ~svtkPlotPie() override;

  /**
   * Update the table cache.
   */
  bool UpdateTableCache(svtkTable* table);

  int Dimensions[4];

  /**
   * The color series to use for the pie.
   */
  svtkSmartPointer<svtkColorSeries> ColorSeries;

  /**
   * Store a well packed set of angles for the wedges of the pie.
   */
  svtkPoints2D* Points;

  /**
   * The point cache is marked dirty until it has been initialized.
   */
  svtkTimeStamp BuildTime;

private:
  svtkPlotPie(const svtkPlotPie&) = delete;
  void operator=(const svtkPlotPie&) = delete;

  svtkPlotPiePrivate* Private;
};

#endif // svtkPlotPie_h
