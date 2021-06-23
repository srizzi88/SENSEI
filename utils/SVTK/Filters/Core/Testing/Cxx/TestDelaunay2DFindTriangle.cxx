/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDelaunay2D_FindTriangle.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCellArray.h"
#include "svtkDelaunay2D.h"

int TestDelaunay2DFindTriangle(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkPoints* newPts = svtkPoints::New();
  newPts->InsertNextPoint(0.650665, -0.325333, 0);
  newPts->InsertNextPoint(-0.325333, 0.650665, 0);
  newPts->InsertNextPoint(-0.325333, -0.325333, 0);
  newPts->InsertNextPoint(0.283966, 0.0265961, 0);
  newPts->InsertNextPoint(0.373199, -0.0478668, 0);
  newPts->InsertNextPoint(-0.325333, 0.535065, 0);

  svtkCellArray* cells = svtkCellArray::New();
  svtkIdType pts[2];
  pts[0] = 3;
  pts[1] = 4;
  cells->InsertNextCell(2, pts);
  pts[0] = 5;
  pts[1] = 3;
  cells->InsertNextCell(2, pts);
  pts[0] = 5;
  pts[1] = 1;
  cells->InsertNextCell(2, pts);
  pts[0] = 1;
  pts[1] = 4;
  cells->InsertNextCell(2, pts);
  pts[0] = 4;
  pts[1] = 0;
  cells->InsertNextCell(2, pts);
  pts[0] = 0;
  pts[1] = 2;
  cells->InsertNextCell(2, pts);
  pts[0] = 2;
  pts[1] = 5;
  cells->InsertNextCell(2, pts);

  svtkPolyData* poly = svtkPolyData::New();
  poly->SetPoints(newPts);
  poly->SetLines(cells);
  newPts->Delete();
  cells->Delete();

  svtkDelaunay2D* del2D = svtkDelaunay2D::New();
  del2D->SetInputData(poly);
  del2D->SetSourceData(poly);
  del2D->SetTolerance(0.0);
  del2D->SetAlpha(0.0);
  del2D->SetOffset(10);
  del2D->BoundingTriangulationOff();
  poly->Delete();
  del2D->Update();

  svtkPolyData* out = del2D->GetOutput();

  svtkIdType numFaces = out->GetNumberOfCells();
  if (numFaces != 5)
  {
    del2D->Delete();
    return EXIT_FAILURE;
  }

  int expected[5][3] = { { 4, 2, 0 }, { 4, 3, 2 }, { 5, 3, 1 }, { 4, 1, 3 }, { 5, 3, 2 } };

  int face[3] = { 0, 0, 0 };
  for (int i = 0; i < numFaces; ++i)
  {
    svtkCell* cell = out->GetCell(i);
    face[0] = cell->GetPointId(0);
    face[1] = cell->GetPointId(1);
    face[2] = cell->GetPointId(2);
    if (face[0] != expected[i][0] || face[1] != expected[i][1] || face[2] != expected[i][2])
    {
      del2D->Delete();
      return EXIT_FAILURE;
    }
  }

  del2D->Delete();
  return EXIT_SUCCESS;
}
