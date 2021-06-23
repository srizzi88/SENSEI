/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSimpleBondPerceiver.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSimpleBondPerceiver.h"

#include "svtkCommand.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMolecule.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOctreePointLocator.h"
#include "svtkPeriodicTable.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTrivialProducer.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedShortArray.h"

#include <vector>

//----------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkSimpleBondPerceiver);

//----------------------------------------------------------------------------
svtkSimpleBondPerceiver::svtkSimpleBondPerceiver()
  : Tolerance(0.45)
  , IsToleranceAbsolute(true)
{
}

//----------------------------------------------------------------------------
svtkSimpleBondPerceiver::~svtkSimpleBondPerceiver() = default;

//----------------------------------------------------------------------------
void svtkSimpleBondPerceiver::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Tolerance: " << this->Tolerance << "\n";
  os << indent << "IsToleranceAbsolute: " << this->IsToleranceAbsolute << "\n";
}

//----------------------------------------------------------------------------
int svtkSimpleBondPerceiver::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkMolecule* input = svtkMolecule::SafeDownCast(svtkDataObject::GetData(inputVector[0]));
  if (!input)
  {
    svtkErrorMacro(<< "Input svtkMolecule does not exists.");
    return 0;
  }

  svtkMolecule* output = svtkMolecule::SafeDownCast(svtkDataObject::GetData(outputVector));
  if (!output)
  {
    svtkErrorMacro(<< "Output svtkMolecule does not exists.");
    return 0;
  }

  // Copy input to output
  output->Initialize();
  output->DeepCopyStructure(input);
  output->ShallowCopyAttributes(input);

  this->ComputeBonds(output);

  return 1;
}

//----------------------------------------------------------------------------
void svtkSimpleBondPerceiver::ComputeBonds(svtkMolecule* molecule)
{
  if (!molecule)
  {
    svtkWarningMacro(<< "svtkMolecule to fill is not defined.");
    return;
  }

  svtkPoints* atomPositions = molecule->GetPoints();

  if (atomPositions->GetNumberOfPoints() == 0)
  {
    // nothing to do.
    return;
  }

  svtkNew<svtkPolyData> moleculePolyData;
  moleculePolyData->SetPoints(atomPositions);
  svtkNew<svtkOctreePointLocator> locator;
  locator->SetDataSet(moleculePolyData.Get());
  locator->BuildLocator();

  svtkUnsignedCharArray* ghostAtoms = molecule->GetAtomGhostArray();
  svtkUnsignedCharArray* ghostBonds = molecule->GetBondGhostArray();

  svtkIdType nbAtoms = molecule->GetNumberOfAtoms();
  svtkNew<svtkIdList> neighborsIdsList;
  svtkNew<svtkPeriodicTable> periodicTable;
  int nbElementsPeriodicTable = periodicTable->GetNumberOfElements();

  /**
   * Main algorithm:
   *  - loop on each atom.
   *  - use locator to determine potential pair: consider atoms in a radius of 2*covalentRadius
   *  - for each potential pair, compute atomic radius (with tolerance) and distance
   *  - if (d < r1 + r2) add a bond. Do not add twice a same bond. Do not create bond between two
   * ghost atoms.
   *  - if one of the two atoms is a ghost, mark bond as ghost
   */
  for (svtkIdType i = 0; i < nbAtoms; i++)
  {
    bool isGhostAtom = (ghostAtoms ? (ghostAtoms->GetTuple1(i) != 0) : false);
    svtkIdType atomicNumber = molecule->GetAtomAtomicNumber(i);

    if (atomicNumber < 1 || atomicNumber > nbElementsPeriodicTable)
    {
      continue;
    }

    double covalentRadius = this->GetCovalentRadiusWithTolerance(periodicTable, atomicNumber);
    double atomPosition[3];
    atomPositions->GetPoint(i, atomPosition);
    neighborsIdsList->SetNumberOfIds(0);
    locator->FindPointsWithinRadius(2 * covalentRadius, atomPosition, neighborsIdsList.Get());

    svtkIdType nbNeighbors = neighborsIdsList->GetNumberOfIds();
    svtkIdType* neighborsPtr = neighborsIdsList->GetPointer(0);
    for (svtkIdType j = 0; j < nbNeighbors; ++j)
    {
      svtkIdType neighId = neighborsPtr[j];
      bool isGhostNeigh = (ghostAtoms ? (ghostAtoms->GetTuple1(neighId) != 0) : false);
      svtkIdType atomicNumberNeigh = molecule->GetAtomAtomicNumber(neighId);

      if (atomicNumberNeigh < 1 || (atomicNumberNeigh > nbElementsPeriodicTable) ||
        (isGhostAtom && isGhostNeigh))
      {
        continue;
      }

      double covalentRadiusNeigh =
        this->GetCovalentRadiusWithTolerance(periodicTable, atomicNumberNeigh);
      double radiusSumSquare =
        (covalentRadius + covalentRadiusNeigh) * (covalentRadius + covalentRadiusNeigh);
      double doubleNeighbourRadiusSquare = 4 * covalentRadiusNeigh * covalentRadiusNeigh;
      double atomPositionNeigh[3];
      molecule->GetAtom(neighId).GetPosition(atomPositionNeigh);
      double distanceSquare = svtkMath::Distance2BetweenPoints(atomPosition, atomPositionNeigh);

      /**
       * Bond may have already been created:
       *  - neighId <= i : we already check bonds for atom 'neighId' in a previous iteration.
       *  - distanceSquare <= doubleNeighbourRadiusSquare : atom 'i' was in the list for potential
       *    pair with neighId.
       *  So if the bond i-neighId is possible, it was added the first time and we can continue.
       *
       * Distance can be to big:
       *  - distanceSquare > radiusSumSquare
       */
      if ((neighId <= i && distanceSquare <= doubleNeighbourRadiusSquare) ||
        distanceSquare > radiusSumSquare)
      {
        continue;
      }

      molecule->AppendBond(i, neighId);
      if (ghostBonds)
      {
        ghostBonds->InsertNextValue(isGhostAtom || isGhostNeigh ? 1 : 0);
      }
    }
  }
}

//----------------------------------------------------------------------------
double svtkSimpleBondPerceiver::GetCovalentRadiusWithTolerance(
  svtkPeriodicTable* table, svtkIdType atomicNumber)
{
  return this->IsToleranceAbsolute ? table->GetCovalentRadius(atomicNumber) + this->Tolerance / 2
                                   : table->GetCovalentRadius(atomicNumber) * this->Tolerance;
}
