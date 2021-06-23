/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalDataExtractDataSets.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHierarchicalDataExtractDataSets
 * @brief   extract a number of datasets
 *
 * Legacy class. Use svtkExtractDataSets instead.
 *
 * @sa
 * svtkExtractDataSets
 */

#ifndef svtkHierarchicalDataExtractDataSets_h
#define svtkHierarchicalDataExtractDataSets_h

#include "svtkExtractDataSets.h"
#include "svtkFiltersExtractionModule.h" // For export macro

struct svtkHierarchicalDataExtractDataSetsInternals;

class SVTKFILTERSEXTRACTION_EXPORT svtkHierarchicalDataExtractDataSets : public svtkExtractDataSets
{
public:
  svtkTypeMacro(svtkHierarchicalDataExtractDataSets, svtkExtractDataSets);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkHierarchicalDataExtractDataSets* New();

protected:
  svtkHierarchicalDataExtractDataSets();
  ~svtkHierarchicalDataExtractDataSets() override;

private:
  svtkHierarchicalDataExtractDataSets(const svtkHierarchicalDataExtractDataSets&) = delete;
  void operator=(const svtkHierarchicalDataExtractDataSets&) = delete;
};

#endif
