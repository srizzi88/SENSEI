/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinearSubdivisionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLinearSubdivisionFilter
 * @brief   generate a subdivision surface using the Linear Scheme
 *
 * svtkLinearSubdivisionFilter is a filter that generates output by
 * subdividing its input polydata. Each subdivision iteration create 4
 * new triangles for each triangle in the polydata.
 *
 * @par Thanks:
 * This work was supported by PHS Research Grant No. 1 P41 RR13218-01
 * from the National Center for Research Resources.
 *
 * @sa
 * svtkInterpolatingSubdivisionFilter svtkButterflySubdivisionFilter
 */

#ifndef svtkLinearSubdivisionFilter_h
#define svtkLinearSubdivisionFilter_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkInterpolatingSubdivisionFilter.h"

class svtkIntArray;
class svtkPointData;
class svtkPoints;
class svtkPolyData;

class SVTKFILTERSMODELING_EXPORT svtkLinearSubdivisionFilter
  : public svtkInterpolatingSubdivisionFilter
{
public:
  //@{
  /**
   * Construct object with NumberOfSubdivisions set to 1.
   */
  static svtkLinearSubdivisionFilter* New();
  svtkTypeMacro(svtkLinearSubdivisionFilter, svtkInterpolatingSubdivisionFilter);
  //@}

protected:
  svtkLinearSubdivisionFilter() {}
  ~svtkLinearSubdivisionFilter() override {}

  int GenerateSubdivisionPoints(svtkPolyData* inputDS, svtkIntArray* edgeData, svtkPoints* outputPts,
    svtkPointData* outputPD) override;

private:
  svtkLinearSubdivisionFilter(const svtkLinearSubdivisionFilter&) = delete;
  void operator=(const svtkLinearSubdivisionFilter&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkLinearSubdivisionFilter.h
