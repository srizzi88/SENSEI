/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DistributedData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test of svtkProjectSphereFilter. It checks the output in here
// and doesn't compare to an image.

#include "svtkArrayCalculator.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPointDataToCellData.h"
#include "svtkProjectSphereFilter.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"

namespace
{
// returns true for success. type is Point or Cell to give
// feedback for errors in the passed in array.
bool CheckFieldData(
  const char* type, svtkDataArray* array, int component, double minValue, double maxValue)
{
  double values[3];
  for (svtkIdType i = 0; i < array->GetNumberOfTuples(); i++)
  {
    array->GetTuple(i, values);
    for (int j = 0; j < 3; j++)
    {
      if (j == component)
      {
        if (values[j] != 0.0 && (values[j] < minValue || values[j] > maxValue))
        {
          svtkGenericWarningMacro("Array type "
            << type << " with name " << array->GetName() << " has bad value of " << values[j]
            << " but should be between " << minValue << " and " << maxValue);
          return false;
        }
      }
      else if (values[j] < -0.001 || values[j] > 0.001)
      {
        svtkGenericWarningMacro("Array type " << type << " with name " << array->GetName()
                                             << " should be 0 but has value of " << values[j]);
        return false;
      }
    }
  }
  return true;
}
}

int TestProjectSphereFilter(int svtkNotUsed(argc), char*[])
{
  int numberOfErrors = 0;

  svtkNew<svtkSphereSource> sphere;
  sphere->SetRadius(1);
  sphere->SetCenter(0, 0, 0);
  sphere->SetThetaResolution(50);
  sphere->SetPhiResolution(50);

  svtkNew<svtkArrayCalculator> calculator;
  calculator->SetInputConnection(sphere->GetOutputPort());
  calculator->SetResultArrayName("result");
  calculator->SetFunction(
    "-coordsY*iHat/sqrt(coordsY^2+coordsX^2)+coordsX*jHat/sqrt(coordsY^2+coordsX^2)");
  calculator->SetAttributeTypeToPointData();
  calculator->AddCoordinateScalarVariable("coordsX", 0);
  calculator->AddCoordinateScalarVariable("coordsY", 1);

  svtkNew<svtkProjectSphereFilter> projectSphere;
  projectSphere->SetCenter(0, 0, 0);
  projectSphere->SetInputConnection(calculator->GetOutputPort());

  svtkNew<svtkPointDataToCellData> pointToCell;
  pointToCell->SetInputConnection(projectSphere->GetOutputPort());
  pointToCell->PassPointDataOn();

  pointToCell->Update();

  svtkDataSet* grid = pointToCell->GetOutput();
  if (grid->GetNumberOfPoints() != 2450)
  {
    svtkGenericWarningMacro(
      "Wrong number of points. There are " << grid->GetNumberOfPoints() << " but should be 2450.");
    numberOfErrors++;
  }
  if (grid->GetNumberOfCells() != 4700)
  {
    svtkGenericWarningMacro(
      "Wrong number of cells. There are " << grid->GetNumberOfCells() << " but should be 4700.");
    numberOfErrors++;
  }

  if (CheckFieldData("Point", grid->GetPointData()->GetArray("result"), 0, .99, 1.01) == false)
  {
    numberOfErrors++;
  }

  if (CheckFieldData("Point", grid->GetPointData()->GetArray("Normals"), 2, .99, 1.01) == false)
  {
    numberOfErrors++;
  }

  if (CheckFieldData("Cell", grid->GetCellData()->GetArray("Normals"), 2, .99, 1.01) == false)
  {
    numberOfErrors++;
  }

  return numberOfErrors;
}
