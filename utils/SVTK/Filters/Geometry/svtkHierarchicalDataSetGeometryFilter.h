/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalDataSetGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHierarchicalDataSetGeometryFilter
 * @brief   extract geometry from hierarchical data
 *
 * Legacy class. Use svtkCompositeDataGeometryFilter instead.
 *
 * @sa
 * svtkCompositeDataGeometryFilter
 */

#ifndef svtkHierarchicalDataSetGeometryFilter_h
#define svtkHierarchicalDataSetGeometryFilter_h

#include "svtkCompositeDataGeometryFilter.h"
#include "svtkFiltersGeometryModule.h" // For export macro

class svtkPolyData;

class SVTKFILTERSGEOMETRY_EXPORT svtkHierarchicalDataSetGeometryFilter
  : public svtkCompositeDataGeometryFilter
{
public:
  static svtkHierarchicalDataSetGeometryFilter* New();
  svtkTypeMacro(svtkHierarchicalDataSetGeometryFilter, svtkCompositeDataGeometryFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkHierarchicalDataSetGeometryFilter();
  ~svtkHierarchicalDataSetGeometryFilter() override;

private:
  svtkHierarchicalDataSetGeometryFilter(const svtkHierarchicalDataSetGeometryFilter&) = delete;
  void operator=(const svtkHierarchicalDataSetGeometryFilter&) = delete;
};

#endif
