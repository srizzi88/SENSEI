/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayStereo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This test verifies that OSPRay can rendering in stereo modes.

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkConeSource.h"
#include "svtkMatrix4x4.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, var) svtkSmartPointer<type> var = svtkSmartPointer<type>::New()

int TestOSPRayStereo(int argc, char* argv[])
{
  bool useGL = false;
  int stereoType = SVTK_STEREO_SPLITVIEWPORT_HORIZONTAL;
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp("-GL", argv[i]))
    {
      cerr << "GL" << endl;
      useGL = true;
    }
    if (!strcmp("SVTK_STEREO_CRYSTAL_EYES", argv[i]))
    {
      cerr << "SVTK_STEREO_CRYSTAL_EYES" << endl;
      stereoType = SVTK_STEREO_CRYSTAL_EYES;
    }
    if (!strcmp("SVTK_STEREO_INTERLACED", argv[i]))
    {
      cerr << "SVTK_STEREO_INTERLACED" << endl;
      stereoType = SVTK_STEREO_INTERLACED;
    }
    if (!strcmp("SVTK_STEREO_RED_BLUE", argv[i]))
    {
      cerr << "SVTK_STEREO_RED_BLUE" << endl;
      stereoType = SVTK_STEREO_RED_BLUE;
    }
    if (!strcmp("SVTK_STEREO_LEFT", argv[i]))
    {
      cerr << "SVTK_STEREO_LEFT" << endl;
      stereoType = SVTK_STEREO_LEFT;
    }
    if (!strcmp("SVTK_STEREO_RIGHT", argv[i]))
    {
      cerr << "SVTK_STEREO_RIGHT" << endl;
      stereoType = SVTK_STEREO_RIGHT;
    }
    if (!strcmp("SVTK_STEREO_DRESDEN", argv[i]))
    {
      cerr << "SVTK_STEREO_DRESDEN" << endl;
      stereoType = SVTK_STEREO_DRESDEN;
    }
    if (!strcmp("SVTK_STEREO_ANAGLYPH", argv[i]))
    {
      cerr << "SVTK_STEREO_ANAGLYPH" << endl;
      stereoType = SVTK_STEREO_ANAGLYPH;
    }
    if (!strcmp("SVTK_STEREO_CHECKERBOARD", argv[i]))
    {
      cerr << "SVTK_STEREO_CHECKERBOARD" << endl;
      stereoType = SVTK_STEREO_CHECKERBOARD;
    }
    if (!strcmp("SVTK_STEREO_SPLITVIEWPORT_HORIZONTAL", argv[i]))
    {
      cerr << "SVTK_STEREO_SPLITVIEWPORT_HORIZONTAL" << endl;
      stereoType = SVTK_STEREO_SPLITVIEWPORT_HORIZONTAL;
    }
    if (!strcmp("SVTK_STEREO_FAKE", argv[i]))
    {
      cerr << "SVTK_STEREO_FAKE" << endl;
      stereoType = SVTK_STEREO_FAKE;
    }
    if (!strcmp("NOSTEREO", argv[i]))
    {
      cerr << "NO STEREO" << endl;
      stereoType = 0;
    }
  }
  double bottomLeft[3] = { -1.0, -1.0, -10.0 };
  double bottomRight[3] = { 1.0, -1.0, -10.0 };
  double topRight[3] = { 1.0, 1.0, -10.0 };

  SVTK_CREATE(svtkSphereSource, sphere1);
  sphere1->SetCenter(0.2, 0.0, -7.0);
  sphere1->SetRadius(0.5);
  sphere1->SetThetaResolution(100);
  sphere1->SetPhiResolution(100);

  SVTK_CREATE(svtkPolyDataMapper, mapper1);
  mapper1->SetInputConnection(sphere1->GetOutputPort());

  SVTK_CREATE(svtkActor, actor1);
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetColor(0.8, 0.8, 0.0);

  SVTK_CREATE(svtkConeSource, cone1);
  cone1->SetCenter(0.0, 0.0, -6.0);
  cone1->SetResolution(100);

  SVTK_CREATE(svtkPolyDataMapper, mapper2);
  mapper2->SetInputConnection(cone1->GetOutputPort());

  SVTK_CREATE(svtkActor, actor2);
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetAmbient(0.1);

  SVTK_CREATE(svtkRenderer, renderer);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->SetAmbient(1.0, 1.0, 1.0);

  svtkSmartPointer<svtkOSPRayPass> osprayPass = svtkSmartPointer<svtkOSPRayPass>::New();
  if (!useGL)
  {
    renderer->SetPass(osprayPass);

    for (int i = 0; i < argc; ++i)
    {
      if (!strcmp(argv[i], "--OptiX"))
      {
        svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
        break;
      }
    }
  }

  SVTK_CREATE(svtkRenderWindow, renwin);
  renwin->AddRenderer(renderer);
  renwin->SetSize(400, 400);
  if (stereoType)
  {
    if (stereoType == SVTK_STEREO_CRYSTAL_EYES)
    {
      renwin->StereoCapableWindowOn();
    }
    renwin->SetStereoType(stereoType);
    renwin->SetStereoRender(1);
  }
  else
  {
    cerr << "NOT STEREO" << endl;
    renwin->SetStereoRender(0);
  }
  renwin->SetMultiSamples(0);

  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  double eyePosition[3] = { 0.0, 0.0, 2.0 };

  svtkCamera* camera = renderer->GetActiveCamera();
  camera->SetScreenBottomLeft(bottomLeft);
  camera->SetScreenBottomRight(bottomRight);
  camera->SetScreenTopRight(topRight);
  camera->SetUseOffAxisProjection(1);
  camera->SetEyePosition(eyePosition);
  camera->SetEyeSeparation(0.05);
  camera->SetPosition(0.0, 0.0, 2.0);
  camera->SetFocalPoint(0.0, 0.0, -6.6);
  camera->SetViewUp(0.0, 1.0, 0.0);
  camera->SetViewAngle(30.0);

  renwin->Render();

  int retVal = svtkRegressionTestImageThreshold(renwin, 25);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = svtkRegressionTester::PASSED;
  }

  return (!retVal);
}
