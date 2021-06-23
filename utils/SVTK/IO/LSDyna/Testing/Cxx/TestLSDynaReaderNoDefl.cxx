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
// .NAME Test of svtkLSDynaReader when the dataset has no deflection data
// .SECTION Description
// Tests the svtkLSDynaReader when nodal deflections are part of the
// simulation state but have not been saved (i.e., the IU flag is 0).

#include "svtkDebugLeaks.h"
#include "svtkLSDynaReader.h"

#include "svtkActor.h"
#include "svtkCompositeDataGeometryFilter.h"
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

int TestLSDynaReaderNoDefl(int argc, char* argv[])
{
  // Read file name.
  char* fname =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/LSDyna/bug_14713/bug_14713.d3plot");

  // Create the reader.
  svtkNew<svtkLSDynaReader> reader;
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  svtkNew<svtkCompositeDataGeometryFilter> geom1;
  geom1->SetInputConnection(0, reader->GetOutputPort(0));

  // Create a mapper.
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(geom1->GetOutputPort());
  mapper->SetScalarModeToUsePointFieldData();

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

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
