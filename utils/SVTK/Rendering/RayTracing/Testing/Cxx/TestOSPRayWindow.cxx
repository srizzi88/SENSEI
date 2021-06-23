/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayScalarBar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkElevationFilter.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkLight.h"
#include "svtkNew.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOSPRayWindowNode.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty2D.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTextProperty.h"
#include "svtkUnsignedCharArray.h"

#include "svtkTestUtilities.h"

int TestOSPRayWindow(int argc, char* argv[])
{
  svtkNew<svtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  svtkNew<svtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort(0));

  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(elev->GetOutputPort(0));
  svtkNew<svtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  svtkSmartPointer<svtkLight> light1 = svtkSmartPointer<svtkLight>::New();

  // Create the RenderWindow, Renderer and all Actors
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  ren1->AddLight(light1);

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", ren1);
      break;
    }
  }

  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sphereActor);
  ren1->SetBackground(.2, .3, .4);
  ren1->SetEnvironmentalBG(.2, .3, .4);

  // render the image
  renWin->SetWindowName("SVTK - Scalar Bar options");
  renWin->SetSize(600, 500);

  svtkNew<svtkOSPRayWindowNode> owindow;
  owindow->SetRenderable(renWin);
  owindow->TraverseAllPasses();

  // now get the result and display it
  const int* size = owindow->GetSize();
  svtkNew<svtkImageData> image;
  image->SetDimensions(size[0], size[1], 1);
  image->GetPointData()->SetScalars(owindow->GetColorBuffer());

  // Create a new image actor and remove the geometry one
  svtkNew<svtkImageActor> imageActor;
  imageActor->GetMapper()->SetInputData(image);
  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(imageActor);

  // Background color white to distinguish image boundary
  renderer->SetBackground(1, 1, 1);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> renderWindowInteractor;
  renderWindowInteractor->SetRenderWindow(renderWindow);
  renderWindow->Render();
  renderer->ResetCamera();
  renderWindow->Render();
  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
