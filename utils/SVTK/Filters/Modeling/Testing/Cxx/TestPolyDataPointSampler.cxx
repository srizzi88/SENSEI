/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyDataPointSampler.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

// If WRITE_RESULT is defined, the result of the surface filter is saved.
//#define WRITE_RESULT

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkMath.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataPointSampler.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkStripper.h"
#include "svtkTestUtilities.h"

int TestPolyDataPointSampler(int argc, char* argv[])
{
  // Standard rendering classes
  svtkRenderer* renderer = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Create a generating polydata
  svtkSphereSource* ss = svtkSphereSource::New();
  ss->SetPhiResolution(25);
  ss->SetThetaResolution(38);
  ss->SetCenter(4.5, 5.5, 5.0);
  ss->SetRadius(2.5);

  // Create multiple samplers to test different parts of the algorithm
  svtkPolyDataPointSampler* sampler = svtkPolyDataPointSampler::New();
  sampler->SetInputConnection(ss->GetOutputPort());
  sampler->SetDistance(0.05);
  sampler->GenerateInteriorPointsOn();

  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(sampler->GetOutputPort());

  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);

  svtkStripper* stripper = svtkStripper::New();
  stripper->SetInputConnection(ss->GetOutputPort());

  svtkPolyDataPointSampler* sampler2 = svtkPolyDataPointSampler::New();
  sampler2->SetInputConnection(stripper->GetOutputPort());
  sampler2->SetDistance(0.05);
  sampler2->GenerateInteriorPointsOn();

  svtkPolyDataMapper* mapper2 = svtkPolyDataMapper::New();
  mapper2->SetInputConnection(sampler2->GetOutputPort());

  svtkActor* actor2 = svtkActor::New();
  actor2->SetMapper(mapper2);
  actor2->AddPosition(5.5, 0, 0);
  actor2->GetProperty()->SetColor(0, 1, 0);

  // Add actors
  renderer->AddActor(actor);
  renderer->AddActor(actor2);

  // Standard testing code.
  renWin->SetSize(500, 250);
  renWin->Render();
  renderer->GetActiveCamera()->Zoom(2);
  renWin->Render();

  int retVal = svtkRegressionTestImageThreshold(renWin, 0.3);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();

  ss->Delete();
  sampler->Delete();
  stripper->Delete();
  sampler2->Delete();
  mapper->Delete();
  mapper2->Delete();
  actor->Delete();
  actor2->Delete();

  return !retVal;
}
