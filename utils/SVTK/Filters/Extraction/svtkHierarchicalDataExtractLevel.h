/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalDataExtractLevel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHierarchicalDataExtractLevel
 * @brief   extract levels between min and max
 *
 * Legacy class. Use svtkExtractLevel instead.
 */

#ifndef svtkHierarchicalDataExtractLevel_h
#define svtkHierarchicalDataExtractLevel_h

#include "svtkExtractLevel.h"
#include "svtkFiltersExtractionModule.h" // For export macro

class SVTKFILTERSEXTRACTION_EXPORT svtkHierarchicalDataExtractLevel : public svtkExtractLevel
{
public:
  svtkTypeMacro(svtkHierarchicalDataExtractLevel, svtkExtractLevel);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkHierarchicalDataExtractLevel* New();

protected:
  svtkHierarchicalDataExtractLevel();
  ~svtkHierarchicalDataExtractLevel() override;

private:
  svtkHierarchicalDataExtractLevel(const svtkHierarchicalDataExtractLevel&) = delete;
  void operator=(const svtkHierarchicalDataExtractLevel&) = delete;
};

#endif
