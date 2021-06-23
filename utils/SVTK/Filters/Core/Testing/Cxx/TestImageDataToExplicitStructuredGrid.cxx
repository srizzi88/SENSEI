/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageDataToExplicitStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test creates a wavelet dataset and creates a explicit grid using
// svtkImageDataToExplicitStructuredGrid

#include "svtkActor.h"
#include "svtkDataSetMapper.h"
#include "svtkImageDataToExplicitStructuredGrid.h"
#include "svtkNew.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTesting.h"

int TestImageDataToExplicitStructuredGrid(int argc, char* argv[])
{
  // Create the sample dataset
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->SetCenter(0.0, 0.0, 0.0);
  wavelet->Update();

  svtkNew<svtkImageDataToExplicitStructuredGrid> esgConvertor;
  esgConvertor->SetInputConnection(wavelet->GetOutputPort());

  svtkNew<svtkDataSetMapper> mapper;
  mapper->SetInputConnection(esgConvertor->GetOutputPort());
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  svtkNew<svtkRenderer> ren;
  ren->AddActor(actor);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
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
