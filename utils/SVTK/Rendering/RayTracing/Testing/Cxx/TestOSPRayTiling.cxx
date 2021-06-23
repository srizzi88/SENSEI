/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayRenderMesh.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can render at resolutions larger than the window
// by rendering and stitching multiple tiles.

#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapper3D.h"
#include "svtkJPEGReader.h"
#include "svtkLight.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPLYReader.h"
#include "svtkPNGWriter.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTexture.h"
#include "svtkWindowToImageFilter.h"

#include "svtkOSPRayTestInteractor.h"

int TestOSPRayTiling(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);
  svtkOSPRayRendererNode::SetSamplesPerPixel(16, renderer);
  svtkOSPRayRendererNode::SetBackgroundMode(2, renderer);

  svtkSmartPointer<svtkLight> l = svtkSmartPointer<svtkLight>::New();
  l->SetLightTypeToHeadlight();
  l->SetIntensity(0.1);
  renderer->AddLight(l);

  // todo: as soon as we get materials, make the bunny reflective
  // to really show off
  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  svtkSmartPointer<svtkPLYReader> polysource = svtkSmartPointer<svtkPLYReader>::New();
  polysource->SetFileName(fileName);

  svtkSmartPointer<svtkPolyDataNormals> normals = svtkSmartPointer<svtkPolyDataNormals>::New();
  normals->SetInputConnection(polysource->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(normals->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);

  renWin->SetSize(400, 400);

  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();
  renderer->SetPass(ospray);
  svtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  svtkSmartPointer<svtkTexture> textr = svtkSmartPointer<svtkTexture>::New();
  svtkSmartPointer<svtkJPEGReader> imgReader = svtkSmartPointer<svtkJPEGReader>::New();
  svtkSmartPointer<svtkImageData> image = svtkSmartPointer<svtkImageData>::New();

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/wintersun.jpg");
  imgReader->SetFileName(fname);
  delete[] fname;
  imgReader->Update();
  textr->SetInputConnection(imgReader->GetOutputPort(0));
  renderer->UseImageBasedLightingOn();
  renderer->SetEnvironmentTexture(textr);

  double up[3] = { 0.0, 1.0, 0.0 };
  double east[3] = { -1.0, 0.0, 0.0 };
  svtkOSPRayRendererNode::SetNorthPole(up, renderer);
  svtkOSPRayRendererNode::SetEastPole(east, renderer);

  renWin->Render();

  svtkSmartPointer<svtkWindowToImageFilter> w2i = svtkSmartPointer<svtkWindowToImageFilter>::New();
  w2i->SetInput(renWin);
  w2i->SetScale(4, 4);
  w2i->Update();

  // svtkSmartPointer<svtkPNGWriter> writer = svtkSmartPointer<svtkPNGWriter>::New();
  // writer->SetFileName("screenshot.png");
  // writer->SetInputConnection(w2i->GetOutputPort());
  // writer->Write();

  // Show stitched image in separate window
  svtkNew<svtkImageActor> imageActor;
  imageActor->GetMapper()->SetInputData(w2i->GetOutput());
  svtkNew<svtkRenderer> ren2;
  ren2->AddActor(imageActor);

  // Background color white to distinguish image boundary
  ren2->SetEnvironmentalBG(1, 1, 1);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(ren2);
  renderWindow->Render();

  svtkSmartPointer<svtkOSPRayTestInteractor> style = svtkSmartPointer<svtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();
  return 0;
}
