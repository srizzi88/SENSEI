/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTDx.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers the 3DConnexion device interface with earth navigation
// interactor style.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkPolyDataMapper.h"

#include "svtkEarthSource.h"
#include "svtkPNMReader.h"
#include "svtkTexture.h"
#include "svtkTexturedSphereSource.h"

#include "svtkCommand.h"
#include "svtkProperty.h"
#include "svtkTDxMotionEventInfo.h"

#include "svtkInteractorStyle.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkTDxInteractorStyleCamera.h"
#include "svtkTDxInteractorStyleGeo.h"
#include "svtkTDxInteractorStyleSettings.h"

const double angleSensitivity = 0.02;
const double translationSensitivity = 0.001;

int TestTDxGeo(int argc, char* argv[])
{
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetUseTDx(true);
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetMultiSamples(0);

  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);
  renWin->Delete();

  svtkRenderer* renderer = svtkRenderer::New();
  renWin->AddRenderer(renderer);
  renderer->Delete();

  // textured earth
  svtkActor* earthActor = svtkActor::New();

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/earth.ppm");
  svtkPNMReader* reader = svtkPNMReader::New();
  reader->SetFileName(fname);
  delete[] fname;

  svtkTexture* earthTexture = svtkTexture::New();
  earthTexture->SetInputConnection(reader->GetOutputPort());
  reader->Delete();
  earthTexture->SetInterpolate(true);
  earthActor->SetTexture(earthTexture);
  earthTexture->Delete();

  svtkPolyDataMapper* earthMapper = svtkPolyDataMapper::New();
  earthActor->SetMapper(earthMapper);
  earthMapper->Delete();

  svtkTexturedSphereSource* tss = svtkTexturedSphereSource::New();
  tss->SetThetaResolution(36); // longitudes
  tss->SetPhiResolution(18);   // latitudes

  earthMapper->SetInputConnection(tss->GetOutputPort());
  tss->Delete();

  // earth contour
  svtkEarthSource* es = svtkEarthSource::New();
  es->SetRadius(0.501);
  es->SetOnRatio(2);
  svtkPolyDataMapper* earth2Mapper = svtkPolyDataMapper::New();
  earth2Mapper->SetInputConnection(es->GetOutputPort());
  es->Delete();
  svtkActor* earth2Actor = svtkActor::New();
  earth2Actor->SetMapper(earth2Mapper);
  earth2Mapper->Delete();

  renderer->AddActor(earthActor);
  earthActor->Delete();
  renderer->AddActor(earth2Actor);
  earth2Actor->Delete();

  renderer->SetBackground(0.1, 0.3, 0.0);
  renWin->SetSize(200, 200);

  renWin->Render();

  renderer->ResetCamera();
  renWin->Render();

  svtkInteractorStyleTrackballCamera* s = svtkInteractorStyleTrackballCamera::New();
  iren->SetInteractorStyle(s);
  s->Delete();

  svtkTDxInteractorStyleGeo* t = svtkTDxInteractorStyleGeo::New();

  s->SetTDxStyle(t);
  t->Delete();

  t->GetSettings()->SetAngleSensitivity(angleSensitivity);
  t->GetSettings()->SetTranslationXSensitivity(translationSensitivity);
  t->GetSettings()->SetTranslationYSensitivity(translationSensitivity);
  t->GetSettings()->SetTranslationZSensitivity(translationSensitivity);

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  iren->Delete();

  return !retVal;
}
