/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotPoints3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotPoints3D
 * @brief   3D scatter plot.
 *
 *
 * 3D scatter plot.
 *
 * @sa
 * svtkPlotLine3D
 * svtkPlotPoints
 *
 */

#ifndef svtkPlotPoints3D_h
#define svtkPlotPoints3D_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkPlot3D.h"

class svtkContext2D;

class SVTKCHARTSCORE_EXPORT svtkPlotPoints3D : public svtkPlot3D
{
public:
  svtkTypeMacro(svtkPlotPoints3D, svtkPlot3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkPlotPoints3D* New();

  /**
   * Paint event for the XY plot, called whenever the chart needs to be drawn
   */
  bool Paint(svtkContext2D* painter) override;

protected:
  svtkPlotPoints3D();
  ~svtkPlotPoints3D() override;

  /**
   * The selected points.
   */
  std::vector<svtkVector3f> SelectedPoints;

  /**
   * The selected points.
   */
  svtkTimeStamp SelectedPointsBuildTime;

private:
  svtkPlotPoints3D(const svtkPlotPoints3D&) = delete;
  void operator=(const svtkPlotPoints3D&) = delete;
};

#endif // svtkPlotPoints3D_h
