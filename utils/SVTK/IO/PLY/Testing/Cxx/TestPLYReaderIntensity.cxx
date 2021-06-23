/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYReaderIntensity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkPLYReaderIntensity
// .SECTION Description
//

#include "svtkSmartPointer.h"

#include "svtkDebugLeaks.h"
#include "svtkPLYReader.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"

#include "svtkWindowToImageFilter.h"

int TestPLYReaderIntensity(int argc, char* argv[])
{
  // Read file name.
  const char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Armadillo.ply");

  // Test if the reader thinks it can open the file.
  int canRead = svtkPLYReader::CanReadFile(fname);
  (void)canRead;

  // Create the reader.
  svtkSmartPointer<svtkPLYReader> reader = svtkSmartPointer<svtkPLYReader>::New();
  reader->SetFileName(fname);
  delete[] fname;

  // Create a mapper.
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->ScalarVisibilityOff();

  // Create the actor.
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  // Basic visualisation.
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderer> ren = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(ren);

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  ren->SetBackground(.2, .3, .5);
  ren->ResetCamera();
  ren->GetActiveCamera()->Azimuth(210);
  ren->GetActiveCamera()->Elevation(30);

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
