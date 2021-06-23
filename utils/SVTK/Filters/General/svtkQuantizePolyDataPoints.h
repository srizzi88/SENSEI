/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQuantizePolyDataPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkQuantizePolyDataPoints
 * @brief   quantizes x,y,z coordinates of points
 *
 * svtkQuantizePolyDataPoints is a subclass of svtkCleanPolyData and
 * inherits the functionality of svtkCleanPolyData with the addition that
 * it quantizes the point coordinates before inserting into the point list.
 * The user should set QFactor to a positive value (0.25 by default) and all
 * {x,y,z} coordinates will be quantized to that grain size.
 *
 * A tolerance of zero is expected, though positive values may be used, the
 * quantization will take place before the tolerance is applied.
 *
 * @warning
 * Merging points can alter topology, including introducing non-manifold
 * forms. Handling of degenerate cells is controlled by switches in
 * svtkCleanPolyData.
 *
 * @warning
 * If you wish to operate on a set of coordinates that has no cells, you must
 * add a svtkPolyVertex cell with all of the points to the PolyData
 * (or use a svtkVertexGlyphFilter) before using the svtkCleanPolyData filter.
 *
 * @sa
 * svtkCleanPolyData
 */

#ifndef svtkQuantizePolyDataPoints_h
#define svtkQuantizePolyDataPoints_h

#include "svtkCleanPolyData.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class SVTKFILTERSGENERAL_EXPORT svtkQuantizePolyDataPoints : public svtkCleanPolyData
{
public:
  static svtkQuantizePolyDataPoints* New();
  svtkTypeMacro(svtkQuantizePolyDataPoints, svtkCleanPolyData);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify quantization grain size. Default is 0.25
   */
  svtkSetClampMacro(QFactor, double, 1E-5, SVTK_FLOAT_MAX);
  svtkGetMacro(QFactor, double);
  //@}

  /**
   * Perform quantization on a point
   */
  void OperateOnPoint(double in[3], double out[3]) override;

  /**
   * Perform quantization on bounds
   */
  void OperateOnBounds(double in[6], double out[6]) override;

protected:
  svtkQuantizePolyDataPoints();
  ~svtkQuantizePolyDataPoints() override {}

  double QFactor;

private:
  svtkQuantizePolyDataPoints(const svtkQuantizePolyDataPoints&) = delete;
  void operator=(const svtkQuantizePolyDataPoints&) = delete;
};

#endif
