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
// .NAME Test of svtkNetCDFCAMReader
// .SECTION Description
// Tests the svtkNetCDFCAMReader.

#include "svtkNetCDFCAMReader.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkGeometryFilter.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"
#include "svtkUnstructuredGrid.h"

int TestNetCDFCAMReader(int argc, char* argv[])
{
  // Read file names.
  char* pointsFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/NetCDF/CAMReaderPoints.nc");
  char* connectivityFileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/NetCDF/CAMReaderConnectivity.nc");

  // Create the reader.
  svtkNew<svtkNetCDFCAMReader> reader;
  reader->SetFileName(pointsFileName);
  reader->SetConnectivityFileName(connectivityFileName);
  delete[] pointsFileName;
  pointsFileName = nullptr;
  delete[] connectivityFileName;
  connectivityFileName = nullptr;
  reader->Update();

  // Convert to PolyData.
  svtkNew<svtkGeometryFilter> geometryFilter;
  geometryFilter->SetInputConnection(reader->GetOutputPort());

  // Create a mapper and LUT.
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geometryFilter->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(205, 250);
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("T");

  // Create the actor.
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  // Basic visualisation.
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkCamera> camera;
  ren->ResetCamera(reader->GetOutput()->GetBounds());
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

  return !retVal;
}
