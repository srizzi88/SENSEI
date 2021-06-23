/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkNew.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLVertexBufferObject.h"
#include "svtkPLYReader.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkRenderWindowInteractor.h"

#include "svtkOpenGLRenderWindow.h"

#include "svtkCullerCollection.h"
#include "svtkOpenVRCamera.h"
#include "svtkTransform.h"

#include "svtkPlaneWidget.h"

#include "svtkTransformPolyDataFilter.h"

#include "svtkLight.h"

#include "svtkOpenVRCamera.h"
#include "svtkOpenVRRenderWindow.h"
#include "svtkOpenVRRenderWindowInteractor.h"
#include "svtkOpenVRRenderer.h"

#include "svtkWin32OpenGLRenderWindow.h"
#include "svtkWin32RenderWindowInteractor.h"

//----------------------------------------------------------------------------
int TestDragon(int argc, char* argv[])
{
  svtkNew<svtkOpenVRRenderer> renderer;
  svtkNew<svtkOpenVRRenderWindow> renderWindow;
  svtkNew<svtkOpenVRRenderWindowInteractor> iren;
  svtkNew<svtkOpenVRCamera> cam;
  renderer->SetShowFloor(true);

  svtkNew<svtkActor> actor;
  renderer->SetBackground(0.2, 0.3, 0.4);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  iren->SetRenderWindow(renderWindow);
  renderer->SetActiveCamera(cam);

  // renderer->UseShadowsOn();

  // crazy frame rate requirement
  // need to look into that at some point
  renderWindow->SetDesiredUpdateRate(350.0);
  iren->SetDesiredUpdateRate(350.0);
  iren->SetStillUpdateRate(350.0);

  renderer->RemoveCuller(renderer->GetCullers()->GetLastItem());

  svtkNew<svtkLight> light;
  light->SetLightTypeToSceneLight();
  light->SetPosition(1.0, 1.0, 1.0);
  renderer->AddLight(light);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);

  svtkNew<svtkTransform> trans;
  trans->Translate(10.0, 20.0, 30.0);
  // trans->Scale(10.0,10.0,10.0);
  svtkNew<svtkTransformPolyDataFilter> tf;
  tf->SetTransform(trans);
  tf->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(tf->GetOutputPort());
  mapper->SetVBOShiftScaleMethod(svtkOpenGLVertexBufferObject::AUTO_SHIFT_SCALE);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);
  //  actor->GetProperty()->SetRepresentationToWireframe();

  // the HMD may not be turned on/etc
  renderWindow->Initialize();
  if (renderWindow->GetHMD())
  {
    renderer->ResetCamera();
    renderWindow->Render();

    int retVal = svtkRegressionTestImage(renderWindow.Get());
    if (retVal == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
    return !retVal;
  }
  return 0;
}
