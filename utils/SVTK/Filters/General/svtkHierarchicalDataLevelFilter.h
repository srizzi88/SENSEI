/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalDataLevelFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHierarchicalDataLevelFilter
 * @brief   generate scalars from levels
 *
 * Legacy class. Use svtkLevelIdScalars instead.
 *
 * @sa
 * svtkLevelIdScalars
 */

#ifndef svtkHierarchicalDataLevelFilter_h
#define svtkHierarchicalDataLevelFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkLevelIdScalars.h"

class SVTKFILTERSGENERAL_EXPORT svtkHierarchicalDataLevelFilter : public svtkLevelIdScalars
{
public:
  svtkTypeMacro(svtkHierarchicalDataLevelFilter, svtkLevelIdScalars);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static svtkHierarchicalDataLevelFilter* New();

protected:
  svtkHierarchicalDataLevelFilter();
  ~svtkHierarchicalDataLevelFilter() override;

private:
  svtkHierarchicalDataLevelFilter(const svtkHierarchicalDataLevelFilter&) = delete;
  void operator=(const svtkHierarchicalDataLevelFilter&) = delete;
};

#endif
