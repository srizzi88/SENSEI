/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProjectedTetrahedraOffscreen.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkColorTransferFunction.h>
#include <svtkConeSource.h>
#include <svtkDoubleArray.h>
#include <svtkImageActor.h>
#include <svtkImageMapper3D.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkPolyData.h>
#include <svtkPolyDataMapper.h>
#include <svtkProjectedTetrahedraMapper.h>
#include <svtkProp3D.h>
#include <svtkProperty.h>
#include <svtkRectilinearGrid.h>
#include <svtkRectilinearGridToTetrahedra.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkTesting.h>
#include <svtkTransform.h>
#include <svtkUnstructuredGrid.h>
#include <svtkVolumeProperty.h>
#include <svtkWindowToImageFilter.h>

// Description
// Tests off-screen rendering of svtkProjectedTetrahedra.

// Creates a cube volume
svtkSmartPointer<svtkVolume> CubeVolume_TetrahedraOffscreen(double r, double g, double b)
{
  // Create the coordinates
  svtkNew<svtkDoubleArray> xArray;
  xArray->InsertNextValue(0.0);
  xArray->InsertNextValue(1.0);
  svtkNew<svtkDoubleArray> yArray;
  yArray->InsertNextValue(0.0);
  yArray->InsertNextValue(1.0);
  svtkNew<svtkDoubleArray> zArray;
  zArray->InsertNextValue(0.0);
  zArray->InsertNextValue(1.0);

  // Create the RectilinearGrid
  svtkNew<svtkRectilinearGrid> grid;
  grid->SetDimensions(2, 2, 2);
  grid->SetXCoordinates(xArray);
  grid->SetYCoordinates(yArray);
  grid->SetZCoordinates(zArray);

  // Obtain an UnstructuredGrid made of tetrahedras
  svtkNew<svtkRectilinearGridToTetrahedra> rectilinearGridToTetrahedra;
  rectilinearGridToTetrahedra->SetInputData(grid);
  rectilinearGridToTetrahedra->Update();

  svtkSmartPointer<svtkUnstructuredGrid> ugrid = rectilinearGridToTetrahedra->GetOutput();

  // Add scalars to the grid
  svtkNew<svtkDoubleArray> scalars;
  for (int i = 0; i < 8; i++)
  {
    scalars->InsertNextValue(0);
  }
  ugrid->GetPointData()->SetScalars(scalars);

  // Volume Rendering Mapper
  svtkNew<svtkProjectedTetrahedraMapper> mapper;
  mapper->SetInputData(ugrid);
  mapper->Update();

  // Create the volume
  svtkSmartPointer<svtkVolume> volume = svtkSmartPointer<svtkVolume>::New();
  volume->SetMapper(mapper);

  // Apply a ColorTransferFunction to the volume
  svtkNew<svtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->AddRGBPoint(0.0, r, g, b);
  volume->GetProperty()->SetColor(colorTransferFunction);

  return volume;
}

// Creates a cone actor
svtkSmartPointer<svtkActor> ConeActor_TetrahedraOffscreen(double r, double g, double b)
{
  // Simple cone mapper
  svtkNew<svtkPolyDataMapper> mapper;
  svtkNew<svtkConeSource> coneSource;
  coneSource->SetCenter(0.0, 0.0, 0.0);
  mapper->SetInputConnection(coneSource->GetOutputPort());

  // Create the actor
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->GetProperty()->SetColor(r, g, b);
  actor->SetMapper(mapper);

  return actor;
}

int TestProjectedTetrahedraOffscreen(int argc, char* argv[])
{
  // Create the props

  // The red cube volume
  svtkSmartPointer<svtkProp3D> volume1 = CubeVolume_TetrahedraOffscreen(1, 0, 0);

  // The blue cube volume
  svtkSmartPointer<svtkProp3D> volume2 = CubeVolume_TetrahedraOffscreen(0, 0, 1);

  // The red cone actor
  svtkSmartPointer<svtkProp3D> actor1 = ConeActor_TetrahedraOffscreen(1, 0, 0);

  // Translate the blue props by (2,2)
  svtkNew<svtkTransform> transform;
  transform->Translate(2, 2, 0);
  volume2->SetUserTransform(transform);

  // Create a renderer, render window, and interactor
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  renderWindow->SetSize(300, 300);

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);

  // Render dummy scene on-screen
  renderWindow->SetOffScreenRendering(false);
  renderer->SetBackground(1, 1, 1);
  renderer->AddVolume(volume1);
  renderer->AddVolume(volume2);
  renderWindow->Render();

  renderer->RemoveVolume(volume1);
  renderer->RemoveVolume(volume2);

  // Render off-screen and grab the rendered image
  renderWindow->SetOffScreenRendering(true);
  renderer->SetBackground(0.4, 0.8, 0.4);
  renderer->AddVolume(volume2);
  renderer->AddActor(actor1);
  renderWindow->Render();
  renderer->ResetCamera();

  svtkNew<svtkWindowToImageFilter> win2image;
  win2image->SetInput(renderWindow);
  win2image->Update();
  svtkImageData* offScreenImage = win2image->GetOutput();

  renderer->RemoveVolume(volume2);
  renderer->RemoveActor(actor1);

  renderWindow->SetOffScreenRendering(false);
  renderWindow->Finalize();
  renderWindow->Start();

  // Render on-screen a texture map of the off-screen rendered image
  svtkNew<svtkImageActor> ia;
  ia->GetMapper()->SetInputData(offScreenImage);
  renderer->AddActor(ia);
  renderer->SetBackground(0, 0, 0);

  renderer->GetActiveCamera()->SetPosition(0, 0, -1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 1);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  renderWindow->Render();

  int retVal = svtkTesting::Test(argc, argv, renderWindow, 20);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
