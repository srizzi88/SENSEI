/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeToAtomBallFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMoleculeToAtomBallFilter.h"

#include "svtkCellArray.h"
#include "svtkMolecule.h"
#include "svtkObjectFactory.h"
#include "svtkPeriodicTable.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSphereSource.h"
#include "svtkUnsignedShortArray.h"

#include <vector>

svtkStandardNewMacro(svtkMoleculeToAtomBallFilter);

//----------------------------------------------------------------------------
svtkMoleculeToAtomBallFilter::svtkMoleculeToAtomBallFilter()
  : Resolution(50)
  , RadiusScale(0.8)
  , RadiusSource(CovalentRadius)
{
}

//----------------------------------------------------------------------------
svtkMoleculeToAtomBallFilter::~svtkMoleculeToAtomBallFilter() = default;

//----------------------------------------------------------------------------
int svtkMoleculeToAtomBallFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkMolecule* input = svtkMolecule::SafeDownCast(svtkDataObject::GetData(inputVector[0]));
  svtkPolyData* output = svtkPolyData::SafeDownCast(svtkDataObject::GetData(outputVector));

  // Need this for radius / color lookups
  svtkPeriodicTable* pTab = svtkPeriodicTable::New();

  // Extract data from input molecule
  svtkIdType numAtoms = input->GetNumberOfAtoms();

  // Prep the output
  output->Initialize();
  svtkCellArray* polys = svtkCellArray::New();
  svtkPoints* points = svtkPoints::New();
  svtkUnsignedShortArray* atomicNums = svtkUnsignedShortArray::New();
  atomicNums->SetName(input->GetAtomicNumberArrayName());

  // Initialize a SphereSource
  svtkSphereSource* sphereSource = svtkSphereSource::New();
  sphereSource->SetThetaResolution(this->Resolution);
  sphereSource->SetPhiResolution(this->Resolution);
  sphereSource->Update();

  // Preallocate memory
  points->Allocate(numAtoms * sphereSource->GetOutput()->GetPoints()->GetNumberOfPoints());
  polys->AllocateEstimate(numAtoms * sphereSource->GetOutput()->GetPolys()->GetNumberOfCells(), 3);
  atomicNums->Allocate(numAtoms * sphereSource->GetOutput()->GetPoints()->GetNumberOfPoints());

  // Initialize some variables for later
  svtkIdType numCellPoints;
  const svtkIdType* cellPoints;
  double scaledRadius;
  unsigned short atomicNum;
  svtkVector3f pos;

  // Build a sphere for each atom and append it's data to the output
  // arrays.
  for (svtkIdType atomInd = 0; atomInd < numAtoms; ++atomInd)
  {
    // Extract atomic number, position
    svtkAtom atom = input->GetAtom(atomInd);
    atomicNum = atom.GetAtomicNumber();
    pos = atom.GetPosition();

    // Get scaled radius:
    switch (this->RadiusSource)
    {
      default:
      case CovalentRadius:
        scaledRadius = this->RadiusScale * pTab->GetCovalentRadius(atomicNum);
        break;
      case VDWRadius:
        scaledRadius = this->RadiusScale * pTab->GetVDWRadius(atomicNum);
        break;
      case UnitRadius:
        scaledRadius = this->RadiusScale /* * 1.0 */;
        break;
    }

    // Make hydrogens slightly larger
    if (atomicNum == 1 && RadiusSource == CovalentRadius)
    {
      scaledRadius *= 1.1;
    }

    // Update sphere source
    sphereSource->SetRadius(scaledRadius);
    sphereSource->SetCenter(pos.Cast<double>().GetData());
    sphereSource->Update();

    // Extract polydata from sphere
    svtkPolyData* sphere = sphereSource->GetOutput();
    svtkPoints* spherePoints = sphere->GetPoints();
    svtkCellArray* spherePolys = sphere->GetPolys();

    // Get offset for the new point IDs that will be added to points
    svtkIdType pointOffset = points->GetNumberOfPoints();
    // Total number of new points
    svtkIdType numPoints = spherePoints->GetNumberOfPoints();

    // Add new points, use atomic number for point scalar data.
    for (svtkIdType i = 0; i < numPoints; ++i)
    {
      points->InsertNextPoint(spherePoints->GetPoint(i));
      atomicNums->InsertNextValue(atomicNum);
    }

    // Add new cells (polygons) that represent the sphere
    spherePolys->InitTraversal();
    while (spherePolys->GetNextCell(numCellPoints, cellPoints) != 0)
    {
      std::vector<svtkIdType> newCellPoints(numCellPoints);
      for (svtkIdType i = 0; i < numCellPoints; ++i)
      {
        // The new point ids should be offset by the pointOffset above
        newCellPoints[i] = cellPoints[i] + pointOffset;
      }
      polys->InsertNextCell(numCellPoints, newCellPoints.data());
    }
  }

  // update output
  output->SetPoints(points);
  output->GetPointData()->SetScalars(atomicNums);
  output->SetPolys(polys);

  // Clean up:
  pTab->Delete();
  polys->Delete();
  points->Delete();
  atomicNums->Delete();
  sphereSource->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void svtkMoleculeToAtomBallFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RadiusSource: ";
  switch (RadiusSource)
  {
    case CovalentRadius:
      os << "CovalentRadius\n";
      break;
    case VDWRadius:
      os << "CovalentRadius\n";
      break;
    case UnitRadius:
      os << "CovalentRadius\n";
      break;
    default:
      os << "Unknown\n";
      break;
  }
  os << indent << "Resolution: " << Resolution << "\n";
  os << indent << "RadiusScale: " << RadiusScale << "\n";
}
