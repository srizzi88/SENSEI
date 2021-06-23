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
#include "svtkCellArray.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPLYReader.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkRenderWindowInteractor.h"

//----------------------------------------------------------------------------
int TestSetZBuffer(int argc, char* argv[])
{
  svtkNew<svtkActor> actor;
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkPolyDataMapper> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  mapper->SetInputConnection(reader->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);
  renderWindow->SetMultiSamples(0);

  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  renderWindow->Render();
  renderWindow->Render();

  std::vector<float> depth;
  depth.reserve(90000);
  for (int i = 0; i < 300; i++)
  {
    for (int j = 0; j < 300; j++)
    {
      depth.push_back((j > 149 ? 0.0 : 1.0));
    }
  }

  renderer->SetPreserveDepthBuffer(1);
  for (int i = 0; i < 4; i++)
  {
    renderWindow->SetZbufferData(0, 0, 299, 299, &(depth[0]));
    renderWindow->Render();
  }
  renderer->SetPreserveColorBuffer(1);

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
