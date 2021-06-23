/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCubeAxes3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2011

#include "svtkBYUReader.h"
#include "svtkCamera.h"
#include "svtkCubeAxesActor.h"
#include "svtkLODActor.h"
#include "svtkLight.h"
#include "svtkNew.h"
#include "svtkOutlineFilter.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestCubeAxesWithZInnerPolys(int argc, char* argv[])
{
  svtkNew<svtkBYUReader> fohe;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/teapot.g");
  fohe->SetGeometryFileName(fname);
  delete[] fname;

  svtkNew<svtkPolyDataNormals> normals;
  normals->SetInputConnection(fohe->GetOutputPort());

  svtkNew<svtkPolyDataMapper> foheMapper;
  foheMapper->SetInputConnection(normals->GetOutputPort());

  svtkNew<svtkLODActor> foheActor;
  foheActor->SetMapper(foheMapper);
  foheActor->GetProperty()->SetDiffuseColor(0.7, 0.3, 0.0);

  svtkNew<svtkOutlineFilter> outline;
  outline->SetInputConnection(normals->GetOutputPort());

  svtkNew<svtkPolyDataMapper> mapOutline;
  mapOutline->SetInputConnection(outline->GetOutputPort());

  svtkNew<svtkActor> outlineActor;
  outlineActor->SetMapper(mapOutline);
  outlineActor->GetProperty()->SetColor(0., 0., 0.);

  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1.0, 100.0);
  camera->SetFocalPoint(0.9, 1.0, 0.0);
  camera->SetPosition(11.63, 6.0, 10.77);

  svtkNew<svtkLight> light;
  light->SetFocalPoint(0.21406, 1.5, 0.0);
  light->SetPosition(8.3761, 4.94858, 4.12505);

  svtkNew<svtkRenderer> ren2;
  ren2->SetActiveCamera(camera);
  ren2->AddLight(light);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren2);
  renWin->SetWindowName("SVTK - Cube Axes custom range");
  renWin->SetSize(600, 600);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren2->AddViewProp(foheActor);
  ren2->AddViewProp(outlineActor);
  ren2->SetBackground(0.1, 0.2, 0.4);

  normals->Update();

  svtkNew<svtkCubeAxesActor> axes2;
  axes2->SetBounds(normals->GetOutput()->GetBounds());
  axes2->SetXAxisRange(20, 300);
  axes2->SetYAxisRange(-0.01, 0.01);
  axes2->SetCamera(ren2->GetActiveCamera());
  axes2->SetXLabelFormat("%6.1f");
  axes2->SetYLabelFormat("%6.1f");
  axes2->SetZLabelFormat("%6.1f");
  axes2->SetScreenSize(15.0);
  axes2->SetFlyModeToClosestTriad();
  axes2->SetCornerOffset(0.0);
  axes2->SetDrawZGridpolys(1);
  axes2->GetZAxesGridpolysProperty()->SetColor(.2, .2, .2);
  axes2->GetZAxesGridpolysProperty()->SetOpacity(.3);

  ren2->AddViewProp(axes2);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
