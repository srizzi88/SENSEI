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
#include "svtkNew.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty2D.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkScalarBarActor.h"
#include "svtkScalarsToColors.h"
#include "svtkSphereSource.h"
#include "svtkTextProperty.h"

#include "svtkTestUtilities.h"

int TestOSPRayScalarBar(int argc, char* argv[])
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

  // Create the RenderWindow, Renderer and all Actors
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkScalarBarActor> scalarBar1 = svtkSmartPointer<svtkScalarBarActor>::New();
  svtkScalarsToColors* lut = sphereMapper->GetLookupTable();
  lut->SetAnnotation(0.0, "Zed");
  lut->SetAnnotation(1.0, "Uno");
  lut->SetAnnotation(0.1, "$\\frac{1}{10}$");
  lut->SetAnnotation(0.125, "$\\frac{1}{8}$");
  lut->SetAnnotation(0.5, "Half");
  scalarBar1->SetTitle("Density");
  scalarBar1->SetLookupTable(lut);
  scalarBar1->DrawAnnotationsOn();
  scalarBar1->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar1->GetPositionCoordinate()->SetValue(.6, .05);
  scalarBar1->SetWidth(0.15);
  scalarBar1->SetHeight(0.5);
  scalarBar1->SetTextPositionToPrecedeScalarBar();
  scalarBar1->GetTitleTextProperty()->SetColor(0., 0., 1.);
  scalarBar1->GetLabelTextProperty()->SetColor(0., 0., 1.);
  scalarBar1->SetDrawFrame(1);
  scalarBar1->GetFrameProperty()->SetColor(0., 0., 0.);
  scalarBar1->SetDrawBackground(1);
  scalarBar1->GetBackgroundProperty()->SetColor(1., 1., 1.);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sphereActor);
  ren1->AddActor(scalarBar1);
  ren1->GradientBackgroundOn();
  ren1->SetBackground(.5, .5, .5);
  ren1->SetBackground2(.0, .0, .0);

  // render the image
  renWin->SetWindowName("SVTK - Scalar Bar options");
  renWin->SetSize(600, 500);
  renWin->SetMultiSamples(0);
  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();
  ren1->SetPass(ospray);

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", ren1);
      break;
    }
  }

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
