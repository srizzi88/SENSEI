/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeToAtomBallFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMoleculeToAtomBallFilter
 * @brief   Generate polydata with spheres
 * representing atoms
 *
 *
 * This filter is used to generate one sphere for each atom in the
 * input svtkMolecule. Each sphere is centered at the atom center and
 * can be scaled using either covalent or van der Waals radii. The
 * point scalars of the output svtkPolyData contains the atomic number
 * of the appropriate atom for color mapping.
 *
 * \note Consider using the faster, simpler svtkMoleculeMapper class,
 * rather than generating polydata manually via these filters.
 *
 * @sa
 * svtkMoleculeMapper svtkMoleculeToBondStickFilter
 */

#ifndef svtkMoleculeToAtomBallFilter_h
#define svtkMoleculeToAtomBallFilter_h

#include "svtkDomainsChemistryModule.h" // For export macro
#include "svtkMoleculeToPolyDataFilter.h"

class svtkMolecule;

class SVTKDOMAINSCHEMISTRY_EXPORT svtkMoleculeToAtomBallFilter : public svtkMoleculeToPolyDataFilter
{
public:
  svtkTypeMacro(svtkMoleculeToAtomBallFilter, svtkMoleculeToPolyDataFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkMoleculeToAtomBallFilter* New();

  enum
  {
    CovalentRadius = 0,
    VDWRadius,
    UnitRadius
  }; // TODO Custom radii from array/fieldData

  svtkGetMacro(RadiusSource, int);
  svtkSetMacro(RadiusSource, int);

  svtkGetMacro(Resolution, int);
  svtkSetMacro(Resolution, int);

  svtkGetMacro(RadiusScale, double);
  svtkSetMacro(RadiusScale, double);

protected:
  svtkMoleculeToAtomBallFilter();
  ~svtkMoleculeToAtomBallFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int Resolution;
  double RadiusScale;
  int RadiusSource;

private:
  svtkMoleculeToAtomBallFilter(const svtkMoleculeToAtomBallFilter&) = delete;
  void operator=(const svtkMoleculeToAtomBallFilter&) = delete;
};

#endif
