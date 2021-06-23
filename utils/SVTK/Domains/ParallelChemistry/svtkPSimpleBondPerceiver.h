/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPSimpleBondPerceiver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPSimpleBondPerceiver
 * @brief   Create a simple guess of a molecule's topology
 *
 *
 * svtkPSimpleBondPerceiver is the parallel version of svtkSimpleBondPerceiver.
 * It computes ghost atoms, ghost bonds and then it calls algorithm from the
 * serial version.
 *
 * @par Thanks:
 * This class has been written by Kitware SAS from an initial work made by
 * Aymeric Pelle from Universite de Technologie de Compiegne, France,
 * and Laurent Colombet and Thierry Carrard from Commissariat a l'Energie
 * Atomique (CEA/DIF).
 */

#ifndef svtkPSimpleBondPerceiver_h
#define svtkPSimpleBondPerceiver_h

#include "svtkDomainsParallelChemistryModule.h" // For export macro
#include "svtkSimpleBondPerceiver.h"

class SVTKDOMAINSPARALLELCHEMISTRY_EXPORT svtkPSimpleBondPerceiver : public svtkSimpleBondPerceiver
{
public:
  static svtkPSimpleBondPerceiver* New();
  svtkTypeMacro(svtkPSimpleBondPerceiver, svtkSimpleBondPerceiver);

protected:
  svtkPSimpleBondPerceiver() = default;
  ~svtkPSimpleBondPerceiver() = default;

  /**
   * Create ghosts level in molecule.
   * Return true if ghosts are correctly initialized.
   */
  bool CreateGhosts(svtkMolecule* molecule);

  /**
   * Compute the bonds. Reimplements Superclass to create ghost before.
   */
  void ComputeBonds(svtkMolecule* molecule) override;

private:
  svtkPSimpleBondPerceiver(const svtkPSimpleBondPerceiver&) = delete; // Not implemented.
  void operator=(const svtkPSimpleBondPerceiver&) = delete;          // Not implemented.
};
#endif
