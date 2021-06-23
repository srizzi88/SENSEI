/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Mace.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor.h"
#include "svtkConeSource.h"
#include "svtkDebugLeaks.h"
#include "svtkGlyph3D.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"

int Mace(int argc, char* argv[])
{
  svtkRenderer* renderer = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  svtkSphereSource* sphere = svtkSphereSource::New();
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  svtkPolyDataMapper* sphereMapper = svtkPolyDataMapper::New();
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  svtkActor* sphereActor = svtkActor::New();
  sphereActor->SetMapper(sphereMapper);

  svtkConeSource* cone = svtkConeSource::New();
  cone->SetResolution(6);

  svtkGlyph3D* glyph = svtkGlyph3D::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  svtkPolyDataMapper* spikeMapper = svtkPolyDataMapper::New();
  spikeMapper->SetInputConnection(glyph->GetOutputPort());

  svtkActor* spikeActor = svtkActor::New();
  spikeActor->SetMapper(spikeMapper);

  renderer->AddActor(sphereActor);
  renderer->AddActor(spikeActor);
  renderer->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);

  // interact with data
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  // Clean up
  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  sphere->Delete();
  sphereMapper->Delete();
  sphereActor->Delete();
  cone->Delete();
  glyph->Delete();
  spikeMapper->Delete();
  spikeActor->Delete();

  return !retVal;
}
