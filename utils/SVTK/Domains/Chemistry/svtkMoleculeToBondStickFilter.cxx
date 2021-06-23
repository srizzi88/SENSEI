/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMoleculeToBondStickFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMoleculeToBondStickFilter.h"

#include "svtkCellArray.h"
#include "svtkCylinderSource.h"
#include "svtkMath.h"
#include "svtkMolecule.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkTransform.h"
#include "svtkUnsignedShortArray.h"

#include <vector>

svtkStandardNewMacro(svtkMoleculeToBondStickFilter);

//----------------------------------------------------------------------------
svtkMoleculeToBondStickFilter::svtkMoleculeToBondStickFilter() = default;

//----------------------------------------------------------------------------
svtkMoleculeToBondStickFilter::~svtkMoleculeToBondStickFilter() = default;

//----------------------------------------------------------------------------
int svtkMoleculeToBondStickFilter::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkMolecule* input = svtkMolecule::SafeDownCast(svtkDataObject::GetData(inputVector[0]));
  svtkPolyData* output = svtkPolyData::SafeDownCast(svtkDataObject::GetData(outputVector));

  // Extract data from input molecule
  svtkIdType numBonds = input->GetNumberOfBonds();

  // Prep the output
  output->Initialize();
  svtkCellArray* polys = svtkCellArray::New();
  svtkPoints* points = svtkPoints::New();
  svtkUnsignedShortArray* bondOrders = svtkUnsignedShortArray::New();
  bondOrders->SetName(input->GetBondOrdersArrayName());

  // Initialize a CylinderSource
  svtkCylinderSource* cylSource = svtkCylinderSource::New();
  cylSource->SetResolution(20);
  cylSource->SetHeight(1.0);
  cylSource->Update();

  // Preallocate memory
  points->Allocate(3 * numBonds * cylSource->GetOutput()->GetPoints()->GetNumberOfPoints());
  polys->AllocateEstimate(numBonds * cylSource->GetOutput()->GetPolys()->GetNumberOfCells(), 3);
  bondOrders->Allocate(3 * numBonds * cylSource->GetOutput()->GetPoints()->GetNumberOfPoints());

  // Create a transform object to map the cylinder source to the bond
  svtkTransform* xform = svtkTransform::New();
  xform->PostMultiply();

  // Declare some variables for later
  svtkIdType numCellPoints;
  const svtkIdType* cellPoints;
  unsigned short bondOrder;
  double bondLength;
  double radius;
  double delta[3];
  double initialDisp[3];
  double rotAngle;
  double rotAxis[3];
  double bondCenter[3];
  double pos1[3], pos2[3];
  // Normalized vector pointing along the cylinder (y axis);
  static const double cylVec[3] = { 0.0, 1.0, 0.0 };
  // Normalized vector pointing along bond
  double bondVec[3];
  // Unit z vector
  static const double unitZ[3] = { 0.0, 0.0, 1.0 };

  // Build a sphere for each atom and append it's data to the output
  // arrays.
  for (svtkIdType bondInd = 0; bondInd < numBonds; ++bondInd)
  {
    // Extract bond info
    svtkBond bond = input->GetBond(bondInd);
    bondOrder = bond.GetOrder();
    bond.GetBeginAtom().GetPosition(pos1);
    bond.GetEndAtom().GetPosition(pos2);

    // Compute additional bond info
    // - Normalized vector in direction of bond
    svtkMath::Subtract(pos2, pos1, bondVec);
    bondLength = svtkMath::Normalize(bondVec);
    // - Axis for cylinder rotation, bondVec [cross] cylVec
    svtkMath::Cross(bondVec, cylVec, rotAxis);
    // Rotation angle
    rotAngle = -svtkMath::DegreesFromRadians(acos(svtkMath::Dot(bondVec, cylVec)));
    // - Center of bond for translation
    svtkMath::Add(pos2, pos1, bondCenter);
    svtkMath::MultiplyScalar(bondCenter, 0.5);

    // Set up delta step vector and bond radius from bond order:
    switch (bondOrder)
    {
      case 1:
      default:
        radius = 0.1;
        delta[0] = 0.0;
        delta[1] = 0.0;
        delta[2] = 0.0;
        initialDisp[0] = 0.0;
        initialDisp[1] = 0.0;
        initialDisp[2] = 0.0;
        break;
      case 2:
        radius = 0.1;
        svtkMath::Cross(bondVec, unitZ, delta);
        // svtkMath::Normalize(delta);
        svtkMath::MultiplyScalar(delta, radius + radius);
        initialDisp[0] = delta[0] * -0.5;
        initialDisp[1] = delta[1] * -0.5;
        initialDisp[2] = delta[2] * -0.5;
        break;
      case 3:
        radius = 0.1;
        svtkMath::Cross(bondVec, unitZ, delta);
        // svtkMath::Normalize(delta);
        svtkMath::MultiplyScalar(delta, radius + radius);
        initialDisp[0] = -delta[0];
        initialDisp[1] = -delta[1];
        initialDisp[2] = -delta[2];
        break;
    }

    // Construct transform
    xform->Identity();
    xform->Scale(radius, bondLength, radius);
    xform->RotateWXYZ(rotAngle, rotAxis);
    xform->Translate(bondCenter[0], bondCenter[1], bondCenter[2]);
    xform->Translate(initialDisp);

    // For each bond order, add a cylinder to output, translate by
    // delta, and repeat.
    for (unsigned short iter = 0; iter < bondOrder; ++iter)
    {
      svtkPolyData* cylinder = cylSource->GetOutput();
      svtkPoints* cylPoints = cylinder->GetPoints();
      svtkCellArray* cylPolys = cylinder->GetPolys();
      xform->TransformPoints(cylPoints, points);

      // Get offset for the new point IDs that will be added to points
      svtkIdType pointOffset = points->GetNumberOfPoints();
      // Total number of new points
      svtkIdType numPoints = cylPoints->GetNumberOfPoints();

      // Use bond order for point scalar data.
      for (svtkIdType i = 0; i < numPoints; ++i)
      {
        bondOrders->InsertNextValue(bondOrder);
      }

      // Add new cells (polygons) that represent the cylinder
      cylPolys->InitTraversal();
      while (cylPolys->GetNextCell(numCellPoints, cellPoints) != 0)
      {
        std::vector<svtkIdType> newCellPoints(numCellPoints);
        for (svtkIdType i = 0; i < numCellPoints; ++i)
        {
          // The new point ids should be offset by the pointOffset above
          newCellPoints[i] = cellPoints[i] + pointOffset;
        }
        polys->InsertNextCell(numCellPoints, newCellPoints.data());
      }

      // Setup for the next cylinder in a multi-bond
      xform->Translate(delta);
    }
  }

  // Release extra memory
  points->Squeeze();
  bondOrders->Squeeze();
  polys->Squeeze();

  // update output
  output->SetPoints(points);
  output->GetPointData()->SetScalars(bondOrders);
  output->SetPolys(polys);

  // Clean up
  xform->Delete();
  polys->Delete();
  points->Delete();
  bondOrders->Delete();
  cylSource->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void svtkMoleculeToBondStickFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
