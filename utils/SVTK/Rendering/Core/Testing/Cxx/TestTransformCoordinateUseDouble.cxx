/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpenGLPolyDataMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME Test of svtkLabelPlacementMapper
// .SECTION Description
// this program tests svtkLabelPlacementMapper which uses a sophisticated algorithm to
// prune labels/icons preventing them from overlapping.

#include <svtkActor2D.h>
#include <svtkCellArray.h>
#include <svtkNew.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper2D.h>
#include <svtkProperty2D.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkTestUtilities.h>

int TestTransformCoordinateUseDouble(int argc, char* argv[])
{
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);

  // Create a box around the renderers
  svtkNew<svtkPolyData> poly;
  svtkNew<svtkPoints> points;

  // Shift the points so they don't fall right between 2 pixels but on the on
  // located on the top right.
  double shift = 0.0002;
  points->InsertNextPoint(0. + shift, 0. + shift, 0); // bottom-left
  points->InsertNextPoint(1. + shift, 0. + shift, 0); // bottom-right
  points->InsertNextPoint(1. + shift, 1. + shift, 0); // top-right
  points->InsertNextPoint(0. + shift, 1. + shift, 0); // top-left

  svtkNew<svtkCellArray> cells;
  cells->InsertNextCell(5);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(2);
  cells->InsertCellPoint(3);
  cells->InsertCellPoint(0);

  poly->SetPoints(points);
  poly->SetLines(cells);

  int i = 6;
  double x = 0.;
  double y = 1. / 8.;
  double width = 1. / 4.;
  double height = 1. / 8.;

  svtkNew<svtkRenderer> emptyRenderer;
  emptyRenderer->SetViewport(0, 0, width, height);
  renderWindow->AddRenderer(emptyRenderer);

  while (--i)
  {
    svtkNew<svtkRenderer> renderer;
    renderer->SetViewport(x, y, x + width, y + height);

    svtkNew<svtkCoordinate> boxCoordinate;
    boxCoordinate->SetCoordinateSystemToNormalizedViewport();
    boxCoordinate->SetViewport(renderer);

    svtkNew<svtkPolyDataMapper2D> polyDataMapper;
    polyDataMapper->SetInputData(poly);
    polyDataMapper->SetTransformCoordinate(boxCoordinate);
    polyDataMapper->SetTransformCoordinateUseDouble(true);

    svtkNew<svtkActor2D> boxActor;
    boxActor->SetMapper(polyDataMapper);

    renderer->AddActor2D(boxActor);

    renderWindow->AddRenderer(renderer);

    if (i % 2)
    {
      x += width;
      y -= height;
      height *= 2.;
    }
    else
    {
      x -= width;
      y += height;
      width *= 2.;
    }
  }

  // Render and interact
  svtkNew<svtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);

  renderWindow->SetMultiSamples(0);
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return !retVal;
}
