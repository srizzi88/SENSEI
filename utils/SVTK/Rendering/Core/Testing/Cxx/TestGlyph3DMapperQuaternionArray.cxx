/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkConeSource.h"
#include "svtkElevationFilter.h"
#include "svtkFloatArray.h"
#include "svtkGlyph3DMapper.h"
#include "svtkInteractorStyleSwitch.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

int TestGlyph3DMapperQuaternionArray(int argc, char* argv[])
{
  int res = 30;
  svtkNew<svtkPlaneSource> plane;
  plane->SetResolution(res, res);

  svtkNew<svtkElevationFilter> colors;
  colors->SetInputConnection(plane->GetOutputPort());
  colors->SetLowPoint(-0.25, -0.25, -0.25);
  colors->SetHighPoint(0.25, 0.25, 0.25);
  colors->Update();

  svtkNew<svtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(colors->GetOutputPort());

  svtkPointData* pointData = svtkDataSet::SafeDownCast(colors->GetOutput())->GetPointData();
  pointData->SetActiveScalars("Elevation");

  svtkFloatArray* elevData = svtkFloatArray::SafeDownCast(pointData->GetArray("Elevation"));

  svtkIdType nbTuples = elevData->GetNumberOfTuples();

  svtkNew<svtkFloatArray> quatData;
  quatData->SetNumberOfComponents(4);
  quatData->SetNumberOfTuples(nbTuples);
  quatData->SetName("Quaternion");

  float* elevPtr = elevData->GetPointer(0);
  float* quatPtr = quatData->GetPointer(0);

  for (svtkIdType i = 0; i < nbTuples; i++)
  {
    float angle = (*elevPtr++) * svtkMath::Pi();
    float s = sin(0.5 * angle);
    float c = cos(0.5 * angle);

    *quatPtr++ = c * c * c + s * s * s;
    *quatPtr++ = s * c * c - c * s * s;
    *quatPtr++ = c * s * c + s * c * s;
    *quatPtr++ = c * c * s - s * s * c;
  }

  pointData->AddArray(quatData);

  svtkNew<svtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  planeActor->GetProperty()->SetRepresentationToWireframe();

  svtkNew<svtkConeSource> squad;
  squad->SetHeight(10.0);
  squad->SetRadius(1.0);
  squad->SetResolution(50);
  squad->SetDirection(0.0, 0.0, 1.0);

  svtkNew<svtkGlyph3DMapper> glypher;
  glypher->SetInputConnection(colors->GetOutputPort());
  glypher->SetOrientationArray("Quaternion");
  glypher->SetOrientationModeToQuaternion();
  glypher->SetScaleFactor(0.01);

  glypher->SetSourceConnection(squad->GetOutputPort());

  svtkNew<svtkActor> glyphActor;

  glyphActor->SetMapper(glypher);

  // Create the rendering stuff

  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> win;
  win->SetMultiSamples(0); // make sure regression images are the same on all platforms
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkInteractorStyleSwitch::SafeDownCast(iren->GetInteractorStyle())
    ->SetCurrentStyleToTrackballCamera();

  iren->SetRenderWindow(win);

  ren->AddActor(planeActor);
  ren->AddActor(glyphActor);
  ren->SetBackground(0.5, 0.5, 0.5);
  win->SetSize(450, 450);
  win->Render();
  ren->GetActiveCamera()->Zoom(1.5);

  win->Render();

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
