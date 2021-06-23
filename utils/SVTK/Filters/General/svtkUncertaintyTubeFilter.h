/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUncertaintyTubeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUncertaintyTubeFilter
 * @brief   generate uncertainty tubes along a polyline
 *
 * svtkUncertaintyTubeFilter is a filter that generates ellipsoidal (in cross
 * section) tubes that follows a polyline. The input is a svtkPolyData with
 * polylines that have associated vector point data. The vector data represents
 * the uncertainty of the polyline in the x-y-z directions.
 *
 * @warning
 * The vector uncertainty values define an axis-aligned ellipsoid at each
 * polyline point. The uncertainty tubes can be envisioned as the
 * interpolation of these ellipsoids between the points defining the
 * polyline (or rather, the interpolation of the cross section of the
 * ellipsoids along the polyline).
 *
 * @sa
 * svtkTensorGlyph svtkStreamTracer
 */

#ifndef svtkUncertaintyTubeFilter_h
#define svtkUncertaintyTubeFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkTubeArray;

class SVTKFILTERSGENERAL_EXPORT svtkUncertaintyTubeFilter : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for printing and obtaining type information for instances of this class.
   */
  svtkTypeMacro(svtkUncertaintyTubeFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Object factory method to instantiate this class.
   */
  static svtkUncertaintyTubeFilter* New();

  //@{
  /**
   * Set / get the number of sides for the tube. At a minimum,
   * the number of sides is 3.
   */
  svtkSetClampMacro(NumberOfSides, int, 3, SVTK_INT_MAX);
  svtkGetMacro(NumberOfSides, int);
  //@}

protected:
  svtkUncertaintyTubeFilter();
  ~svtkUncertaintyTubeFilter() override;

  // Integrate data
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int BuildTubes(svtkPointData* pd, svtkPointData* outPD, svtkCellData* cd, svtkCellData* outCD,
    svtkPolyData* output);

  // array of uncertainty tubes
  svtkTubeArray* Tubes;
  int NumberOfTubes;

  // number of sides of tube
  int NumberOfSides;

private:
  svtkUncertaintyTubeFilter(const svtkUncertaintyTubeFilter&) = delete;
  void operator=(const svtkUncertaintyTubeFilter&) = delete;
};

#endif
