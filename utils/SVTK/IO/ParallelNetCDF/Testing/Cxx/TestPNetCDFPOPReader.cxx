/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestProStarReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkPNetCDFPOPReader
// .SECTION Description
// Tests the svtkPNetCDFPOPReader.

#include "svtkPNetCDFPOPReader.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkGeometryFilter.h"
#include "svtkMPIController.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRectilinearGrid.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"

int TestPNetCDFPOPReader(int argc, char* argv[])
{
  svtkMPIController* controller = svtkMPIController::New();

  controller->Initialize(&argc, &argv, 0);
  controller->SetGlobalController(controller);

  // Read file name.
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/NetCDF/test.pop.nc");

  // Create the reader.
  svtkSmartPointer<svtkPNetCDFPOPReader> reader = svtkSmartPointer<svtkPNetCDFPOPReader>::New();
  reader->SetFileName(fname);
  reader->SetStride(2, 3, 4);
  delete[] fname;
  reader->Update();
  svtkRectilinearGrid* grid = svtkRectilinearGrid::SafeDownCast(reader->GetOutput());
  grid->GetPointData()->SetScalars(grid->GetPointData()->GetArray("DYE01"));

  // Convert to PolyData.
  svtkGeometryFilter* geometryFilter = svtkGeometryFilter::New();
  geometryFilter->SetInputConnection(reader->GetOutputPort());

  // Create a mapper.
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(geometryFilter->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);

  // Basic visualisation.
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  svtkRenderer* ren = svtkRenderer::New();
  renWin->AddRenderer(ren);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  svtkCamera* camera = ren->GetActiveCamera();
  ren->ResetCamera(grid->GetBounds());
  camera->Zoom(8);

  ren->AddActor(actor);
  ren->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);

  // interact with data
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  actor->Delete();
  mapper->Delete();
  geometryFilter->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();

  controller->Finalize(0);
  controller->Delete();

  return !retVal;
}
