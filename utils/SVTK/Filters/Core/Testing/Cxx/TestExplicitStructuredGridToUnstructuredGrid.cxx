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
// This test creates an explicit grid and convert it to unstructured grid using
// svtkExplicitStructuredGridToUnstructuredGrid

#include "svtkActor.h"
#include "svtkCellData.h"
#include "svtkDataSetMapper.h"
#include "svtkExplicitStructuredGridToUnstructuredGrid.h"
#include "svtkImageDataToExplicitStructuredGrid.h"
#include "svtkNew.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTesting.h"
#include "svtkUnstructuredGrid.h"

int TestExplicitStructuredGridToUnstructuredGrid(int argc, char* argv[])
{
  // Create the sample dataset
  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->SetCenter(0.0, 0.0, 0.0);
  wavelet->Update();

  svtkNew<svtkImageDataToExplicitStructuredGrid> esgConvertor;
  esgConvertor->SetInputConnection(wavelet->GetOutputPort());

  svtkNew<svtkExplicitStructuredGridToUnstructuredGrid> ugConvertor;
  ugConvertor->SetInputConnection(esgConvertor->GetOutputPort());
  ugConvertor->Update();

  svtkCellData* cellData = ugConvertor->GetOutput()->GetCellData();
  if (!cellData->GetArray("BLOCK_I") || !cellData->GetArray("BLOCK_I") ||
    !cellData->GetArray("BLOCK_I"))
  {
    std::cout << "Missing expected arrays" << std::endl;
    return EXIT_FAILURE;
  }

  svtkNew<svtkDataSetMapper> mapper;
  mapper->SetInputConnection(ugConvertor->GetOutputPort());
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
