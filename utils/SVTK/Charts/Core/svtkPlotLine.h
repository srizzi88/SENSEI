/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotLine.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotLine
 * @brief   Class for drawing an XY line plot given two columns from
 * a svtkTable.
 *
 *
 *
 */

#ifndef svtkPlotLine_h
#define svtkPlotLine_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkPlotPoints.h"

class SVTKCHARTSCORE_EXPORT svtkPlotLine : public svtkPlotPoints
{
public:
  svtkTypeMacro(svtkPlotLine, svtkPlotPoints);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static svtkPlotLine* New();

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

  //@{
  /**
   * Turn on/off flag to control whether the points define a poly line
   * (true) or multiple line segments (false).
   * If true (default), a segment is drawn between each points
   * (e.g. [P1P2, P2P3, P3P4...].) If false, a segment is drawn for each pair
   * of points (e.g. [P1P2, P3P4,...].)
   */
  svtkSetMacro(PolyLine, bool);
  svtkGetMacro(PolyLine, bool);
  svtkBooleanMacro(PolyLine, bool);
  //@}

protected:
  svtkPlotLine();
  ~svtkPlotLine() override;

  /**
   * Poly line (true) or line segments(false).
   */
  bool PolyLine;

private:
  svtkPlotLine(const svtkPlotLine&) = delete;
  void operator=(const svtkPlotLine&) = delete;
};

#endif // svtkPlotLine_h
