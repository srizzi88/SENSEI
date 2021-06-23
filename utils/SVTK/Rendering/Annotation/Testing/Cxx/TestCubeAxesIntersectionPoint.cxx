/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCubeAxesOrientedBoundingBox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkCamera.h"
#include "svtkCubeAxesActor.h"
#include "svtkLODActor.h"
#include "svtkLight.h"
#include "svtkMath.h"
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
#include "svtkTextProperty.h"

//----------------------------------------------------------------------------
int TestCubeAxesIntersectionPoint(int argc, char* argv[])
{
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(1.0, 100.0);
  camera->SetFocalPoint(1.26612, -0.81045, 1.24353);
  camera->SetPosition(-5.66214, -2.58773, 11.243);

  svtkNew<svtkLight> light;
  light->SetFocalPoint(0.21406, 1.5, 0.0);
  light->SetPosition(8.3761, 4.94858, 4.12505);

  svtkNew<svtkRenderer> ren2;
  ren2->SetActiveCamera(camera);
  ren2->AddLight(light);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren2);
  renWin->SetWindowName("Cube Axes");
  renWin->SetSize(600, 600);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren2->SetBackground(0.1, 0.2, 0.4);

  double baseX[3] = { 1, 1, 0 };
  double baseY[3] = { 0, 1, 1 };
  double baseZ[3] = { 1, 0, 1 };

  svtkMath::Normalize(baseX);
  svtkMath::Normalize(baseY);
  svtkMath::Normalize(baseZ);

  svtkNew<svtkCubeAxesActor> axes;
  axes->SetUseOrientedBounds(1);
  axes->SetOrientedBounds(-1, 1, -1.5, 1.5, 0, 4);
  axes->SetAxisBaseForX(baseX);
  axes->SetAxisBaseForY(baseY);
  axes->SetAxisBaseForZ(baseZ);
  axes->SetCamera(ren2->GetActiveCamera());
  axes->SetXLabelFormat("%6.1f");
  axes->SetYLabelFormat("%6.1f");
  axes->SetZLabelFormat("%6.1f");
  axes->SetScreenSize(15.);
  axes->SetFlyModeToClosestTriad();
  axes->SetAxisOrigin(-1, -0.25, 1);
  axes->SetUseAxisOrigin(1);
  axes->SetCornerOffset(.0);

  // Use red color for X axis
  axes->GetXAxesLinesProperty()->SetColor(1., 0., 0.);
  axes->GetTitleTextProperty(0)->SetColor(1., 0., 0.);
  axes->GetLabelTextProperty(0)->SetColor(.8, 0., 0.);

  // Use green color for Y axis
  axes->GetYAxesLinesProperty()->SetColor(0., 1., 0.);
  axes->GetTitleTextProperty(1)->SetColor(0., 1., 0.);
  axes->GetLabelTextProperty(1)->SetColor(0., .8, 0.);

  ren2->AddViewProp(axes);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  cout << camera->GetFocalPoint()[0] << ", " << camera->GetFocalPoint()[1] << ", "
       << camera->GetFocalPoint()[2] << endl;
  cout << camera->GetPosition()[0] << ", " << camera->GetPosition()[1] << ", "
       << camera->GetPosition()[2] << endl;

  return !retVal;
}
