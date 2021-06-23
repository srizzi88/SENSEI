/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotLine3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkPlotLine3D
 * @brief   Class for drawing an XYZ line plot given three columns from
 * a svtkTable.
 *
 *
 * This class draws points with a line between them given three column from a
 * svtkTable in a svtkChartXYZ.
 *
 * @sa
 * svtkPlotPoints3D
 * svtkPlotLine
 *
 */

#ifndef svtkPlotLine3D_h
#define svtkPlotLine3D_h

#include "svtkChartsCoreModule.h" // For export macro
#include "svtkPlotPoints3D.h"

class SVTKCHARTSCORE_EXPORT svtkPlotLine3D : public svtkPlotPoints3D
{
public:
  svtkTypeMacro(svtkPlotLine3D, svtkPlotPoints3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 3D Chart object.
   */
  static svtkPlotLine3D* New();

  /**
   * Paint event for the XYZ plot, called whenever the chart needs to be drawn.
   */
  bool Paint(svtkContext2D* painter) override;

protected:
  svtkPlotLine3D();
  ~svtkPlotLine3D() override;

private:
  svtkPlotLine3D(const svtkPlotLine3D&) = delete;
  void operator=(const svtkPlotLine3D&) = delete;
};

#endif // svtkPlotLine3D_h
