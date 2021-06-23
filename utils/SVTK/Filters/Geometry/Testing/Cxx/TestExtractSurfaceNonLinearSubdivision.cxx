/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkXMLUnstructuredGridReader.h"

int TestExtractSurfaceNonLinearSubdivision(int argc, char* argv[])
{
  // Basic visualisation.
  svtkNew<svtkRenderer> ren;
  ren->SetBackground(0, 0, 0);

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(ren);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->SetSize(300, 300);

  svtkNew<svtkXMLUnstructuredGridReader> reader;
  char* filename = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadraticTetra01.vtu");
  reader->SetFileName(filename);
  delete[] filename;
  filename = nullptr;

  svtkNew<svtkDataSetSurfaceFilter> extract_surface;
  extract_surface->SetInputConnection(reader->GetOutputPort());
  extract_surface->SetNonlinearSubdivisionLevel(4);

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(extract_surface->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SelectColorArray("scalars");
  mapper->SetScalarModeToUsePointFieldData();

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  ren->AddActor(actor);
  ren->ResetCamera();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
