/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredPointsGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkStructuredPointsGeometryFilter
 * @brief   obsolete class
 *
 * svtkStructuredPointsGeometryFilter has been renamed to
 * svtkImageDataGeometryFilter
 */

#ifndef svtkStructuredPointsGeometryFilter_h
#define svtkStructuredPointsGeometryFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkImageDataGeometryFilter.h"

class SVTKFILTERSGEOMETRY_EXPORT svtkStructuredPointsGeometryFilter
  : public svtkImageDataGeometryFilter
{
public:
  svtkTypeMacro(svtkStructuredPointsGeometryFilter, svtkImageDataGeometryFilter);

  /**
   * Construct with initial extent of all the data
   */
  static svtkStructuredPointsGeometryFilter* New();

protected:
  svtkStructuredPointsGeometryFilter();
  ~svtkStructuredPointsGeometryFilter() override {}

private:
  svtkStructuredPointsGeometryFilter(const svtkStructuredPointsGeometryFilter&) = delete;
  void operator=(const svtkStructuredPointsGeometryFilter&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkStructuredPointsGeometryFilter.h
