/*=========================================================================

Program:   Visualization Toolkit
Module:    TestCubeAxesWithGridlines.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2011

#include "svtkActor.h"
#include "svtkAxisActor.h"
#include "svtkCamera.h"
#include "svtkCoordinate.h"
#include "svtkCubeAxesActor.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkProperty2D.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkTextProperty.h"

#include "svtkCubeAxesActor.h"

int TestCubeAxes2DMode(int argc, char* argv[])
{
  // Create plane source
  svtkSmartPointer<svtkPlaneSource> plane = svtkSmartPointer<svtkPlaneSource>::New();
  plane->SetXResolution(10);
  plane->SetYResolution(10);

  // Create plane mapper
  svtkSmartPointer<svtkPolyDataMapper> planeMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  planeMapper->SetInputConnection(plane->GetOutputPort());
  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();

  // Create plane actor
  svtkSmartPointer<svtkActor> planeActor = svtkSmartPointer<svtkActor>::New();
  planeActor->SetMapper(planeMapper);
  planeActor->GetProperty()->SetColor(.5, .5, .5);

  // Create edge mapper and actor
  svtkSmartPointer<svtkPolyDataMapper> edgeMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  edgeMapper->SetInputConnection(plane->GetOutputPort());
  // move the lines behind the axes a tad
  edgeMapper->SetRelativeCoincidentTopologyLineOffsetParameters(0, 2);

  // Create edge actor
  svtkSmartPointer<svtkActor> edgeActor = svtkSmartPointer<svtkActor>::New();
  edgeActor->SetMapper(edgeMapper);
  edgeActor->GetProperty()->SetColor(.0, .0, .0);
  edgeActor->GetProperty()->SetRepresentationToWireframe();

  // Create renderer
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->SetBackground(1., 1., 1.);
  renderer->GetActiveCamera()->SetFocalPoint(.0, .0, .0);
  renderer->GetActiveCamera()->SetPosition(.0, .0, 2.5);

  // Create cube axes actor
  svtkSmartPointer<svtkCubeAxesActor> axes = svtkSmartPointer<svtkCubeAxesActor>::New();
  axes->SetCamera(renderer->GetActiveCamera());
  axes->SetBounds(-.5, .5, -.5, .5, 0., 0.);
  axes->SetCornerOffset(.0);
  axes->SetXAxisVisibility(1);
  axes->SetYAxisVisibility(1);
  axes->SetZAxisVisibility(0);
  axes->SetUse2DMode(1);

  // Deactivate LOD for all axes
  axes->SetEnableDistanceLOD(0);
  axes->SetEnableViewAngleLOD(0);

  // Use red color for X axis
  axes->GetXAxesLinesProperty()->SetColor(1., 0., 0.);
  axes->GetTitleTextProperty(0)->SetColor(1., 0., 0.);
  axes->GetLabelTextProperty(0)->SetColor(1., 0., 0.);

  // Use green color for Y axis
  axes->GetYAxesLinesProperty()->SetColor(0., 1., 0.);
  axes->GetTitleTextProperty(1)->SetColor(0., 1., 0.);
  axes->GetLabelTextProperty(1)->SetColor(0., 1., 0.);

  // Add all actors to renderer
  renderer->AddActor(planeActor);
  renderer->AddActor(edgeActor);
  renderer->AddActor(axes);

  // Create render window and interactor
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  renderWindow->SetSize(800, 600);
  renderWindow->SetMultiSamples(0);
  svtkSmartPointer<svtkRenderWindowInteractor> interactor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  interactor->SetRenderWindow(renderWindow);

  // Render and possibly interact
  renderWindow->Render();
  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    interactor->Start();
  }

  return !retVal;
}
