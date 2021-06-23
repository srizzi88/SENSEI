/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotGrid
 * @brief   takes care of drawing the plot grid
 *
 *
 * The svtkPlotGrid is drawn in screen coordinates. It is usually one of the
 * first elements of a chart to be drawn, and will generally be obscured
 * by all other elements of the chart. It builds up its own plot locations
 * from the parameters of the x and y axis of the plot.
 */

#ifndef svtkPlotGrid_h
#define svtkPlotGrid_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkContextItem.h"

class svtkStdString;
class svtkContext2D;
class svtkPoints2D;
class svtkAxis;

class SVTKCHARTSCORE_EXPORT svtkPlotGrid : public svtkContextItem
{
public:
  svtkTypeMacro(svtkPlotGrid, svtkContextItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static svtkPlotGrid* New();

  /**
   * Set the X axis of the grid.
   */
  virtual void SetXAxis(svtkAxis* axis);

  /**
   * Set the X axis of the grid.
   */
  virtual void SetYAxis(svtkAxis* axis);

  /**
   * Paint event for the axis, called whenever the axis needs to be drawn
   */
  bool Paint(svtkContext2D* painter) override;

protected:
  svtkPlotGrid();
  ~svtkPlotGrid() override;

  //@{
  /**
   * The svtkAxis objects are used to figure out where the grid lines should be
   * drawn.
   */
  svtkAxis* XAxis;
  svtkAxis* YAxis;
  //@}

private:
  svtkPlotGrid(const svtkPlotGrid&) = delete;
  void operator=(const svtkPlotGrid&) = delete;
};

#endif // svtkPlotGrid_h
