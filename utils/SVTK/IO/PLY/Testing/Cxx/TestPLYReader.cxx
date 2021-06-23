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
// .NAME Test of svtkPLYReader
// .SECTION Description
//

#include "svtkDebugLeaks.h"
#include "svtkPLYReader.h"

#include "svtkActor.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStringArray.h"
#include "svtkTestUtilities.h"

#include "svtkWindowToImageFilter.h"

int TestPLYReader(int argc, char* argv[])
{
  // Read file name.
  const char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");

  // Test if the reader thinks it can open the file.
  int canRead = svtkPLYReader::CanReadFile(fname);
  (void)canRead;

  // Create the reader.
  svtkPLYReader* reader = svtkPLYReader::New();
  reader->SetFileName(fname);
  reader->Update();
  delete[] fname;

  svtkStringArray* comments = reader->GetComments();
  if (comments->GetNumberOfValues() != 2)
  {
    cerr << "Error: expected 2 comments, found " << comments->GetNumberOfValues() << endl;
    return EXIT_FAILURE;
  }
  if (comments->GetValue(0) != "zipper output" || comments->GetValue(1) != "modified by flipply")
  {
    cerr << "Error: comment strings differ from expected " << comments->GetValue(0) << "; "
         << comments->GetValue(1) << endl;
    return EXIT_FAILURE;
  }

  // Create a mapper.
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(reader->GetOutputPort());
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
  reader->Delete();
  renWin->Delete();
  ren->Delete();
  iren->Delete();

  return !retVal;
}
