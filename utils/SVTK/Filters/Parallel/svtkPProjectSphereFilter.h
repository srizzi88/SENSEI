/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPProjectSphereFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPProjectSphereFilter
 * @brief   A filter to 'unroll' a sphere.  The
 * unroll longitude is -180.
 *
 *
 */

#ifndef svtkPProjectSphereFilter_h
#define svtkPProjectSphereFilter_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkProjectSphereFilter.h"

class SVTKFILTERSPARALLEL_EXPORT svtkPProjectSphereFilter : public svtkProjectSphereFilter
{
public:
  svtkTypeMacro(svtkPProjectSphereFilter, svtkProjectSphereFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPProjectSphereFilter* New();

protected:
  svtkPProjectSphereFilter();
  ~svtkPProjectSphereFilter() override;

  /**
   * Parallel part of the algorithm to figure out the closest point
   * to the centerline (i.e. line connecting -90 latitude to 90 latitude)
   * if we don't build cells using points at the poles.
   */
  void ComputePointsClosestToCenterLine(double, svtkIdList*) override;

  /**
   * If TranslateZ is true then this is the method that computes
   * the amount to translate.
   */
  double GetZTranslation(svtkPointSet* input) override;

private:
  svtkPProjectSphereFilter(const svtkPProjectSphereFilter&) = delete;
  void operator=(const svtkPProjectSphereFilter&) = delete;
};

#endif // svtkPProjectSphereFilter_h
