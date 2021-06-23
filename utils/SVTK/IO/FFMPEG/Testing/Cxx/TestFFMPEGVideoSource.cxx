/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFFMPEGVideoSource.cxx

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
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTexture.h"

#include "svtkRegressionTestImage.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkTestUtilities.h"

#include "svtkLookupTable.h"

#include "svtkFFMPEGVideoSource.h"

int TestFFMPEGVideoSource(int argc, char* argv[])
{
  svtkNew<svtkActor> actor;
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkPolyDataMapper> mapper;
  renderer->SetBackground(0.2, 0.3, 0.4);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/tracktor.webm");

  svtkNew<svtkFFMPEGVideoSource> video;
  video->SetFileName(fileName);
  delete[] fileName;

  svtkNew<svtkTexture> texture;
  texture->SetInputConnection(video->GetOutputPort());
  actor->SetTexture(texture);

  svtkNew<svtkPlaneSource> plane;
  mapper->SetInputConnection(plane->GetOutputPort());
  actor->SetMapper(mapper);

  video->Initialize();
  int fsize[3];
  video->GetFrameSize(fsize);
  plane->SetOrigin(0, 0, 0);
  plane->SetPoint1(fsize[0], 0, 0);
  plane->SetPoint2(0, fsize[1], 0);
  renderWindow->Render();

  for (int i = 0; i < 10; ++i)
  {
    video->Grab();
    renderWindow->Render();
  }

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
