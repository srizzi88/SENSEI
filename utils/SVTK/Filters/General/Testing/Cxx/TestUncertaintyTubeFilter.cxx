/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestUncertaintyTubeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test creates some polylines with uncertainty values.

#include "svtkUncertaintyTubeFilter.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkDelaunay2D.h"
#include "svtkDoubleArray.h"
#include "svtkMath.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShrinkPolyData.h"
#include "svtkSmartPointer.h"
#include "svtkTriangleFilter.h"

int TestUncertaintyTubeFilter(int, char*[])
{
  svtkSmartPointer<svtkPoints> newPts = svtkSmartPointer<svtkPoints>::New();
  newPts->SetNumberOfPoints(10);
  newPts->SetPoint(0, 10, 10, 0);
  newPts->SetPoint(1, 10, 10, 2);
  newPts->SetPoint(2, 10, 10, 4);
  newPts->SetPoint(3, 10, 10, 8);
  newPts->SetPoint(4, 10, 10, 12);
  newPts->SetPoint(5, 1, 1, 2);
  newPts->SetPoint(6, 1, 2, 3);
  newPts->SetPoint(7, 1, 4, 3);
  newPts->SetPoint(8, 1, 8, 4);
  newPts->SetPoint(9, 1, 16, 5);

  svtkMath::RandomSeed(1177);
  svtkSmartPointer<svtkDoubleArray> s = svtkSmartPointer<svtkDoubleArray>::New();
  s->SetNumberOfComponents(1);
  s->SetNumberOfTuples(10);
  svtkSmartPointer<svtkDoubleArray> v = svtkSmartPointer<svtkDoubleArray>::New();
  v->SetNumberOfComponents(3);
  v->SetNumberOfTuples(10);
  for (int i = 0; i < 10; i++)
  {
    s->SetTuple1(i, svtkMath::Random(0, 1));
    double x = svtkMath::Random(0.0, 2);
    double y = svtkMath::Random(0.0, 2);
    double z = svtkMath::Random(0.0, 2);
    v->SetTuple3(i, x, y, z);
  }

  svtkSmartPointer<svtkCellArray> lines = svtkSmartPointer<svtkCellArray>::New();
  lines->AllocateEstimate(2, 5);
  lines->InsertNextCell(5);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  lines->InsertCellPoint(4);
  lines->InsertNextCell(5);
  lines->InsertCellPoint(5);
  lines->InsertCellPoint(6);
  lines->InsertCellPoint(7);
  lines->InsertCellPoint(8);
  lines->InsertCellPoint(9);

  svtkSmartPointer<svtkPolyData> pd = svtkSmartPointer<svtkPolyData>::New();
  pd->SetPoints(newPts);
  pd->SetLines(lines);
  pd->GetPointData()->SetScalars(s);
  pd->GetPointData()->SetVectors(v);

  svtkSmartPointer<svtkUncertaintyTubeFilter> utf = svtkSmartPointer<svtkUncertaintyTubeFilter>::New();
  utf->SetInputData(pd);
  utf->SetNumberOfSides(8);

  svtkSmartPointer<svtkTriangleFilter> tf = svtkSmartPointer<svtkTriangleFilter>::New();
  tf->SetInputConnection(utf->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  // mapper->SetInputConnection( utf->GetOutputPort() );
  mapper->SetInputConnection(tf->GetOutputPort());

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();
  ren->AddActor(actor);

  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  ren->GetActiveCamera()->SetPosition(1, 1, 1);
  ren->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  ren->ResetCamera();

  iren->Initialize();

  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
