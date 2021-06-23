/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkDoubleArray.h"
#include "svtkExecutive.h"
#include "svtkGlyph3D.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestErrorObserver.h"
#include "svtkTestUtilities.h"

static bool TestGlyph3D_WithBadArray()
{
  svtkSmartPointer<svtkDoubleArray> vectors = svtkSmartPointer<svtkDoubleArray>::New();
  vectors->SetName("Normals");
  vectors->SetNumberOfComponents(4);
  vectors->InsertNextTuple4(1, 1, 1, 1);

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  svtkSmartPointer<svtkPolyData> polydata = svtkSmartPointer<svtkPolyData>::New();
  polydata->SetPoints(points);

  polydata->GetPointData()->AddArray(vectors);

  svtkSmartPointer<svtkPolyData> glyph = svtkSmartPointer<svtkPolyData>::New();

  svtkSmartPointer<svtkConeSource> glyphSource = svtkSmartPointer<svtkConeSource>::New();

  svtkSmartPointer<svtkGlyph3D> glyph3D = svtkSmartPointer<svtkGlyph3D>::New();
  glyph3D->SetSourceConnection(glyphSource->GetOutputPort());
  glyph3D->SetInputData(polydata);
  glyph3D->SetInputArrayToProcess(1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "Normals");
  glyph3D->SetVectorModeToUseVector();
  svtkSmartPointer<svtkTest::ErrorObserver> errorObserver1 =
    svtkSmartPointer<svtkTest::ErrorObserver>::New();
  svtkSmartPointer<svtkTest::ErrorObserver> errorObserver2 =
    svtkSmartPointer<svtkTest::ErrorObserver>::New();
  glyph3D->AddObserver(svtkCommand::ErrorEvent, errorObserver1);
  glyph3D->GetExecutive()->AddObserver(svtkCommand::ErrorEvent, errorObserver2);
  glyph3D->Update();
  int status = errorObserver1->CheckErrorMessage("svtkDataArray Normals has more than 3 components");
  status += errorObserver2->CheckErrorMessage("Algorithm svtkGlyph3D");
  return true;
}

static bool TestGlyph3D_WithoutSource()
{
  svtkNew<svtkPoints> points;
  points->InsertNextPoint(0, 0, 0);
  svtkNew<svtkPolyData> polydata;
  polydata->SetPoints(points);

  svtkNew<svtkGlyph3D> glyph3D;
  glyph3D->SetInputData(polydata);
  glyph3D->Update();

  return true;
}

int TestGlyph3D(int argc, char* argv[])
{
  if (!TestGlyph3D_WithBadArray())
  {
    return EXIT_FAILURE;
  }

  if (!TestGlyph3D_WithoutSource())
  {
    return EXIT_FAILURE;
  }

  svtkSmartPointer<svtkDoubleArray> vectors = svtkSmartPointer<svtkDoubleArray>::New();
  vectors->SetName("Normals");
  vectors->SetNumberOfComponents(2);
  vectors->InsertNextTuple2(1, 1);
  vectors->InsertNextTuple2(1, 0);
  vectors->InsertNextTuple2(0, 1);

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 1, 1);
  points->InsertNextPoint(2, 2, 2);

  svtkSmartPointer<svtkPolyData> polydata = svtkSmartPointer<svtkPolyData>::New();
  polydata->SetPoints(points);

  polydata->GetPointData()->AddArray(vectors);

  svtkSmartPointer<svtkPolyData> glyph = svtkSmartPointer<svtkPolyData>::New();

  svtkSmartPointer<svtkConeSource> glyphSource = svtkSmartPointer<svtkConeSource>::New();

  svtkSmartPointer<svtkGlyph3D> glyph3D = svtkSmartPointer<svtkGlyph3D>::New();
  glyph3D->SetSourceConnection(glyphSource->GetOutputPort());
  glyph3D->SetInputData(polydata);
  glyph3D->SetInputArrayToProcess(1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "Normals");
  glyph3D->SetVectorModeToUseVector();
  glyph3D->Update();

  // Visualize

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyph3D->GetOutputPort());

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();
  ren->SetBackground(0, 0, 0);
  ren->AddActor(actor);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);

  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  renWin->AddRenderer(ren);
  renWin->SetSize(300, 300);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
