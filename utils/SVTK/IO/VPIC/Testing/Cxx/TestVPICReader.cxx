/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkVPICReader
// .SECTION Description
// Tests the svtkVPICReader.

#include "svtkDebugLeaks.h"
#include "svtkVPICReader.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkLookupTable.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"

#include "svtkPNGWriter.h"
#include "svtkWindowToImageFilter.h"

#include "svtkNew.h"

int TestVPICReader(int argc, char* argv[])
{
  // Read file name.
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/VPIC/global.vpc");

  // Create the reader.
  svtkNew<svtkVPICReader> reader;
  reader->SetFileName(fname);
  reader->EnableAllPointArrays();
  reader->Update();
  delete[] fname;

  svtkNew<svtkDataSetSurfaceFilter> geom1;
  geom1->SetInputConnection(0, reader->GetOutputPort(0));

  // Create a mapper.
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geom1->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("Charge Density(Hhydro)");
  mapper->SetScalarRange(0.06743, 1.197);

  // Create the actor.
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  // Basic visualisation.
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);

  ren->SetBackground(0, 0, 0);
  renWin->SetSize(300, 300);

  // interact with data
  renWin->Render();
  ren->GetActiveCamera()->Roll(45);
  ren->GetActiveCamera()->Azimuth(45);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
