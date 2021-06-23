/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestUnstructuredGridToExplicitStructuredGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This test read a unstructured grid and creates a explicit grid using
// svtkUnstructuredGridToExplicitStructuredGrid

#include "svtkActor.h"
#include "svtkDataSetMapper.h"
#include "svtkNew.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkUnstructuredGridToExplicitStructuredGrid.h"
#include "svtkXMLUnstructuredGridReader.h"

int TestUnstructuredGridToExplicitStructuredGrid(int argc, char* argv[])
{
  svtkNew<svtkXMLUnstructuredGridReader> reader;
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/explicitStructuredGrid.vtu");
  reader->SetFileName(fname);
  delete[] fname;
  reader->Update();

  svtkNew<svtkUnstructuredGridToExplicitStructuredGrid> esgConvertor;
  esgConvertor->SetInputConnection(reader->GetOutputPort());
  esgConvertor->SetWholeExtent(0, 5, 0, 13, 0, 3);
  esgConvertor->SetInputArrayToProcess(0, 0, 0, 1, "BLOCK_I");
  esgConvertor->SetInputArrayToProcess(1, 0, 0, 1, "BLOCK_J");
  esgConvertor->SetInputArrayToProcess(2, 0, 0, 1, "BLOCK_K");
  esgConvertor->Update();

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
