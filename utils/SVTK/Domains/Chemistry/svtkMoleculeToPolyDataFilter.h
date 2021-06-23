/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeToPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMoleculeToPolyDataFilter
 * @brief   abstract filter class
 *
 * svtkMoleculeToPolyDataFilter is an abstract filter class whose
 * subclasses take as input datasets of type svtkMolecule and
 * generate polygonal data on output.
 */

#ifndef svtkMoleculeToPolyDataFilter_h
#define svtkMoleculeToPolyDataFilter_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkMolecule;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkMoleculeToPolyDataFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkMoleculeToPolyDataFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkMolecule* GetInput();

protected:
  svtkMoleculeToPolyDataFilter();
  ~svtkMoleculeToPolyDataFilter() override;

  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkMoleculeToPolyDataFilter(const svtkMoleculeToPolyDataFilter&) = delete;
  void operator=(const svtkMoleculeToPolyDataFilter&) = delete;
};

#endif
