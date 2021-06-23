/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestNamedComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkArrayCalculator.h"
#include "svtkCellData.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkThreshold.h"
#include "svtkUnstructuredGrid.h"

int TestNamedComponents(int, char*[])
{
  int rval = 0;

  svtkIdType numPoints = 20;
  svtkIdType numVerts = 5;
  svtkIdType numLines = 8;
  svtkIdType numTriangles = 3;
  svtkIdType numStrips = 2;
  svtkIdType numCells = numVerts + numLines + numTriangles + numStrips;
  svtkIdType i;

  svtkIdTypeArray* pointCoords = svtkIdTypeArray::New();
  const char pcName[] = "point coords";
  pointCoords->SetName(pcName);
  pointCoords->SetNumberOfComponents(3); // num points + point ids
  pointCoords->SetNumberOfTuples(numPoints);
  pointCoords->SetComponentName(0, "XLOC");
  pointCoords->SetComponentName(1, "YLOC");
  pointCoords->SetComponentName(2, "ZLOC");

  svtkPoints* points = svtkPoints::New();
  points->SetNumberOfPoints(numPoints);
  for (i = 0; i < numPoints; i++)
  {
    double loc[3] = { static_cast<double>(i), static_cast<double>(i * i), 0.0 };
    points->InsertPoint(i, loc);
    pointCoords->InsertTuple(i, loc);
  }
  svtkSmartPointer<svtkPolyData> poly = svtkSmartPointer<svtkPolyData>::New();
  poly->AllocateExact(numCells, numCells);
  poly->SetPoints(points);
  poly->GetPointData()->AddArray(pointCoords);
  pointCoords->Delete();
  points->Delete();

  for (i = 0; i < numVerts; i++)
  {
    poly->InsertNextCell(SVTK_VERTEX, 1, &i);
  }

  for (i = 0; i < numLines; i++)
  {
    svtkIdType pts[2] = { i, i + 1 };
    poly->InsertNextCell(SVTK_LINE, 2, pts);
  }

  for (i = 0; i < numTriangles; i++)
  {
    svtkIdType pts[3] = { 0, i + 1, i + 2 };
    poly->InsertNextCell(SVTK_TRIANGLE, 3, pts);
  }

  for (i = 0; i < numStrips; i++)
  {
    svtkIdType pts[3] = { 0, i + 1, i + 2 };
    poly->InsertNextCell(SVTK_TRIANGLE_STRIP, 3, pts);
  }

  svtkIntArray* cellIndex = svtkIntArray::New();
  const char ctName[] = "scalars";
  cellIndex->SetName(ctName);
  cellIndex->SetNumberOfComponents(1);
  cellIndex->SetNumberOfTuples(numCells);
  cellIndex->SetComponentName(0, "index");
  for (i = 0; i < numCells; i++)
  {
    cellIndex->SetValue(i, i);
  }
  poly->GetCellData()->SetScalars(cellIndex);
  cellIndex->Delete();

  svtkIdTypeArray* cellPoints = svtkIdTypeArray::New();
  const char cpName[] = "cell points";
  cellPoints->SetName(cpName);
  cellPoints->SetNumberOfComponents(4); // num points + point ids
  cellPoints->SetNumberOfTuples(numCells);

  cellPoints->SetComponentName(0, "NumberOfPoints");
  cellPoints->SetComponentName(1, "X_ID");
  cellPoints->SetComponentName(2, "Y_ID");
  cellPoints->SetComponentName(3, "Z_ID");

  for (i = 0; i < numCells; i++)
  {
    svtkIdType npts;
    const svtkIdType* pts;
    poly->GetCellPoints(i, npts, pts);
    svtkIdType data[4] = { npts, pts[0], 0, 0 };
    for (svtkIdType j = 1; j < npts; j++)
    {
      data[j + 1] = pts[j];
    }
    cellPoints->SetTypedTuple(i, data);
  }

  poly->GetCellData()->AddArray(cellPoints);
  cellPoints->Delete();

  poly->BuildCells();

  svtkSmartPointer<svtkThreshold> thresh = svtkSmartPointer<svtkThreshold>::New();
  thresh->SetInputData(poly);
  thresh->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, svtkDataSetAttributes::SCALARS);

  thresh->ThresholdBetween(0, 10);
  thresh->Update();

  svtkSmartPointer<svtkUnstructuredGrid> out = thresh->GetOutput();

  if (out == nullptr)
  {
    svtkGenericWarningMacro("threshold failed.");
    return 1;
  }

  // the arrays should have been changed so get them again...
  cellIndex = svtkArrayDownCast<svtkIntArray>(out->GetCellData()->GetArray(ctName));
  cellPoints = svtkArrayDownCast<svtkIdTypeArray>(out->GetCellData()->GetArray(cpName));

  // confirm component names are intact
  if (strcmp(cellIndex->GetComponentName(0), "index") != 0)
  {
    svtkGenericWarningMacro("threshold failed to maintain component name on cell scalars.");
    return 1;
  }

  if (strcmp(cellPoints->GetComponentName(0), "NumberOfPoints") != 0 ||
    strcmp(cellPoints->GetComponentName(1), "X_ID") != 0 ||
    strcmp(cellPoints->GetComponentName(2), "Y_ID") != 0 ||
    strcmp(cellPoints->GetComponentName(3), "Z_ID") != 0)
  {
    svtkGenericWarningMacro("threshold failed to maintain component names on point property.");
    return 1;
  }

  // Test component names with the calculator
  svtkSmartPointer<svtkArrayCalculator> calc = svtkSmartPointer<svtkArrayCalculator>::New();
  calc->SetInputData(poly);
  calc->SetAttributeTypeToPointData();
  // Add coordinate scalar and vector variables
  calc->AddCoordinateScalarVariable("coordsX", 0);
  calc->AddScalarVariable("point coords_YLOC", "point coords", 1);
  calc->SetFunction("coordsX + point coords_YLOC");
  calc->SetResultArrayName("Result");
  calc->Update();

  return rval;
}
