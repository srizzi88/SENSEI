/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkShrinkPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkShrinkPolyData
 * @brief   shrink cells composing PolyData
 *
 * svtkShrinkPolyData shrinks cells composing a polygonal dataset (e.g.,
 * vertices, lines, polygons, and triangle strips) towards their centroid.
 * The centroid of a cell is computed as the average position of the
 * cell points. Shrinking results in disconnecting the cells from
 * one another. The output dataset type of this filter is polygonal data.
 *
 * During execution the filter passes its input cell data to its
 * output. Point data attributes are copied to the points created during the
 * shrinking process.
 *
 * @warning
 * It is possible to turn cells inside out or cause self intersection
 * in special cases.
 * Users should use the svtkTriangleFilter to triangulate meshes that
 * contain triangle strips.
 *
 * @sa
 * svtkShrinkFilter
 */

#ifndef svtkShrinkPolyData_h
#define svtkShrinkPolyData_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkShrinkPolyData : public svtkPolyDataAlgorithm
{
public:
  static svtkShrinkPolyData* New();
  svtkTypeMacro(svtkShrinkPolyData, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the fraction of shrink for each cell.
   */
  svtkSetClampMacro(ShrinkFactor, double, 0.0, 1.0);
  //@}

  //@{
  /**
   * Get the fraction of shrink for each cell.
   */
  svtkGetMacro(ShrinkFactor, double);
  //@}

protected:
  svtkShrinkPolyData(double sf = 0.5);
  ~svtkShrinkPolyData() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  double ShrinkFactor;

private:
  svtkShrinkPolyData(const svtkShrinkPolyData&) = delete;
  void operator=(const svtkShrinkPolyData&) = delete;
};

#endif
