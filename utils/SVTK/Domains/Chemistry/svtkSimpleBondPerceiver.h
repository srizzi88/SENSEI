/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimpleBondPerceiver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSimpleBondPerceiver
 * @brief   Create a simple guess of a molecule's
 * topology
 *
 *
 *
 * svtkSimpleBondPerceiver performs a simple check of all interatomic distances
 * and adds a single bond between atoms that are reasonably close. If the
 * interatomic distance is less than the sum of the two atom's covalent radii
 * plus a tolerance, a single bond is added.
 *
 *
 * @warning
 * This algorithm does not consider valences, hybridization, aromaticity, or
 * anything other than atomic separations. It will not produce anything other
 * than single bonds.
 */

#ifndef svtkSimpleBondPerceiver_h
#define svtkSimpleBondPerceiver_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkMoleculeAlgorithm.h"

class svtkDataSet;
class svtkMolecule;
class svtkPeriodicTable;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkSimpleBondPerceiver : public svtkMoleculeAlgorithm
{
public:
  static svtkSimpleBondPerceiver* New();
  svtkTypeMacro(svtkSimpleBondPerceiver, svtkMoleculeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the tolerance used in the comparisons. (Default: 0.45)
   */
  svtkSetMacro(Tolerance, float);
  svtkGetMacro(Tolerance, float);
  //@}

  //@{
  /**
   * Set/Get if the tolerance is absolute (i.e. added to radius)
   * or not (i.e. multiplied with radius). Default is true.
   */
  svtkGetMacro(IsToleranceAbsolute, bool);
  svtkSetMacro(IsToleranceAbsolute, bool);
  //@}

protected:
  svtkSimpleBondPerceiver();
  ~svtkSimpleBondPerceiver() override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Compute the bonds of input molecule.
   */
  virtual void ComputeBonds(svtkMolecule* molecule);

  /**
   * Get the covalent radius corresponding to atomic number, modulated by Tolerance.
   * Tolerance is multiplied if IsToleranceAbsolute is false.
   * Half Tolerance is added if IsToleranceAbsolute is true (for backward compatibility)
   */
  double GetCovalentRadiusWithTolerance(svtkPeriodicTable* table, svtkIdType atomicNumber);

  float Tolerance;
  bool IsToleranceAbsolute;

private:
  svtkSimpleBondPerceiver(const svtkSimpleBondPerceiver&) = delete;
  void operator=(const svtkSimpleBondPerceiver&) = delete;
};

#endif
