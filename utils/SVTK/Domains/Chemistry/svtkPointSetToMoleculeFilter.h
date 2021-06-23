/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointSetToMoleculeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * @class svtkPointSetToMoleculeFilter
 * @brief Converts a pointset into a molecule.
 *
 * svtkPointSetToMoleculeFilter is a filter that takes a svtkPointSet as input
 * and generates a svtkMolecule.
 * Each point of the given svtkPointSet will become an atom of the svtkMolecule.
 * The svtkPointSet should provide a point data array (default is scalar one)
 * to specify the atomic number of each atom.
 */

#ifndef svtkPointSetToMoleculeFilter_h
#define svtkPointSetToMoleculeFilter_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkMoleculeAlgorithm.h"

class SVTKDOMAINSCHEMISTRY_EXPORT svtkPointSetToMoleculeFilter : public svtkMoleculeAlgorithm
{
public:
  static svtkPointSetToMoleculeFilter* New();
  svtkTypeMacro(svtkPointSetToMoleculeFilter, svtkMoleculeAlgorithm);

  //@{
  /**
   * Get/Set if the filter should look for lines in input cells and convert them
   * into bonds.
   * default is ON.
   */
  svtkGetMacro(ConvertLinesIntoBonds, bool);
  svtkSetMacro(ConvertLinesIntoBonds, bool);
  svtkBooleanMacro(ConvertLinesIntoBonds, bool);
  //@}

protected:
  svtkPointSetToMoleculeFilter();
  ~svtkPointSetToMoleculeFilter() override = default;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  bool ConvertLinesIntoBonds;

private:
  svtkPointSetToMoleculeFilter(const svtkPointSetToMoleculeFilter&) = delete;
  void operator=(const svtkPointSetToMoleculeFilter&) = delete;
};

#endif
