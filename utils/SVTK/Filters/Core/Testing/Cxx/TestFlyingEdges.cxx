/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestFlyingEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test creates a wavelet dataset and creates isosurfaces using
// svtkFlyingEdges3D

#include "svtkActor.h"
#include "svtkFlyingEdges3D.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTesting.h"

int TestFlyingEdges(int argc, char* argv[])
{
  // Create the sample dataset
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-63, 64, -63, 64, -63, 64);
  wavelet->SetCenter(0.0, 0.0, 0.0);
  wavelet->Update();

  svtkNew<svtkFlyingEdges3D> flyingEdges;
  flyingEdges->SetInputConnection(wavelet->GetOutputPort());
  flyingEdges->GenerateValues(6, 128.0, 225.0);
  flyingEdges->ComputeNormalsOn();
  flyingEdges->ComputeGradientsOn();
  flyingEdges->ComputeScalarsOn();
  flyingEdges->SetArrayComponent(0);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(flyingEdges->GetOutputPort());
  mapper->SetScalarRange(128, 225);
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  svtkNew<svtkRenderer> ren;
  ren->AddActor(actor);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(399, 401);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren->ResetCamera();
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
