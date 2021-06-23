/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProgrammableGlyph.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkActor.h>
#include <svtkCellArray.h>
#include <svtkConeSource.h>
#include <svtkCubeSource.h>
#include <svtkFloatArray.h>
#include <svtkPointData.h>
#include <svtkPoints.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProgrammableGlyphFilter.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkSphereSource.h>

void CalcGlyph(void* arg)
{

  svtkProgrammableGlyphFilter* glyphFilter = (svtkProgrammableGlyphFilter*)arg;

  if (!glyphFilter)
  {
    std::cerr << "glyphFilter is not valid!" << std::endl;
  }
  double pointCoords[3];
  glyphFilter->GetPoint(pointCoords);

  std::cout << "Calling CalcGlyph for point " << glyphFilter->GetPointId() << std::endl;
  std::cout << "Point coords are: " << pointCoords[0] << " " << pointCoords[1] << " "
            << pointCoords[2] << std::endl;

  if (glyphFilter->GetPointId() == 0)
  {
    // Normal use case: should produce cone
    svtkSmartPointer<svtkConeSource> coneSource = svtkSmartPointer<svtkConeSource>::New();
    coneSource->SetCenter(pointCoords);
    glyphFilter->SetSourceConnection(coneSource->GetOutputPort());
  }
  else if (glyphFilter->GetPointId() == 1)
  {
    // nullptr SourceConnection, valid SourceData: should produce cube
    svtkSmartPointer<svtkCubeSource> cubeSource = svtkSmartPointer<svtkCubeSource>::New();
    cubeSource->SetCenter(pointCoords);
    cubeSource->Update();
    glyphFilter->SetSourceConnection(nullptr);
    glyphFilter->SetSourceData(cubeSource->GetOutput());
  }
  else if (glyphFilter->GetPointId() == 2)
  {
    // Normal use case: should produce sphere
    svtkSmartPointer<svtkSphereSource> sphereSource = svtkSmartPointer<svtkSphereSource>::New();
    sphereSource->SetCenter(pointCoords);
    glyphFilter->SetSourceConnection(sphereSource->GetOutputPort());
  }
  else
  {
    // nullptr SourceConnection and nullptr SourceData: should produce nothing
    glyphFilter->SetSourceConnection(nullptr);
    glyphFilter->SetSourceData(nullptr);
  }
}

int TestProgrammableGlyph(int, char*[])
{
  // Create points
  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(5, 0, 0);
  points->InsertNextPoint(10, 0, 0);
  points->InsertNextPoint(15, 0, 0);

  // Combine into a polydata
  svtkSmartPointer<svtkPolyData> polydata = svtkSmartPointer<svtkPolyData>::New();
  polydata->SetPoints(points);

  svtkSmartPointer<svtkProgrammableGlyphFilter> glyphFilter =
    svtkSmartPointer<svtkProgrammableGlyphFilter>::New();
  glyphFilter->SetInputData(polydata);
  glyphFilter->SetGlyphMethod(CalcGlyph, glyphFilter);
  // need a default glyph, but this should not be used
  svtkSmartPointer<svtkConeSource> coneSource = svtkSmartPointer<svtkConeSource>::New();
  coneSource->Update();
  glyphFilter->SetSourceData(coneSource->GetOutput());

  // Create a mapper and actor
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyphFilter->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  // Create a renderer, render window, and interactor
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Add the actor to the scene
  renderer->AddActor(actor);
  renderer->SetBackground(.2, .3, .4);

  // Render and interact
  renderWindow->Render();
  renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
