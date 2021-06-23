/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeToBondStickFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMoleculeToBondStickFilter
 * @brief   Generate polydata with cylinders
 * representing bonds
 */

#ifndef svtkMoleculeToBondStickFilter_h
#define svtkMoleculeToBondStickFilter_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkMoleculeToPolyDataFilter.h"

class svtkMolecule;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkMoleculeToBondStickFilter : public svtkMoleculeToPolyDataFilter
{
public:
  svtkTypeMacro(svtkMoleculeToBondStickFilter, svtkMoleculeToPolyDataFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkMoleculeToBondStickFilter* New();

protected:
  svtkMoleculeToBondStickFilter();
  ~svtkMoleculeToBondStickFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkMoleculeToBondStickFilter(const svtkMoleculeToBondStickFilter&) = delete;
  void operator=(const svtkMoleculeToBondStickFilter&) = delete;
};

#endif
