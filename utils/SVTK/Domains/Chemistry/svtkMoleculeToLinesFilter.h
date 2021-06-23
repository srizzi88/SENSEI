/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeToLinesFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMoleculeToLinesFilter
 * @brief Convert a molecule into a simple polydata with lines.
 *
 * svtkMoleculeToLinesFilter is a filter class that takes svtkMolecule as input and
 * generates polydata on output.
 * Conversion is done following this rules:
 *  - 1 atom == 1 point
 *  - 1 bond == 1 line (cell of type SVTK_LINE)
 *  - atom data is copied as point data
 *  - bond data is copied as cell data
 */

#ifndef svtkMoleculeToLinesFilter_h
#define svtkMoleculeToLinesFilter_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkMoleculeToPolyDataFilter.h"

class SVTKDOMAINSCHEMISTRY_EXPORT svtkMoleculeToLinesFilter : public svtkMoleculeToPolyDataFilter
{
public:
  static svtkMoleculeToLinesFilter* New();
  svtkTypeMacro(svtkMoleculeToLinesFilter, svtkMoleculeToPolyDataFilter);

protected:
  svtkMoleculeToLinesFilter() = default;
  ~svtkMoleculeToLinesFilter() override = default;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkMoleculeToLinesFilter(const svtkMoleculeToLinesFilter&) = delete;
  void operator=(const svtkMoleculeToLinesFilter&) = delete;
};

#endif
