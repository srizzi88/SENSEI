/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVRMLNormals.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkVRMLImporter.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

int TestVRMLNormals(int argc, char* argv[])
{
  // Now create the RenderWindow, Renderer and Interactor
  svtkRenderer* ren1 = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  svtkVRMLImporter* importer = svtkVRMLImporter::New();
  importer->SetRenderWindow(renWin);

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/WineGlass.wrl");
  importer->SetFileName(fname);
  importer->Read();

  delete[] fname;

  renWin->SetSize(400, 400);

  // render the image
  iren->Initialize();

  // This starts the event loop and as a side effect causes an initial render.
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Delete everything
  importer->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
