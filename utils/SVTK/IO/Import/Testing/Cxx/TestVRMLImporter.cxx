/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestVRMLImporter.cxx

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

#include "svtkTestUtilities.h"

// This is testing a bug in svtkVRMLImporter where the importer
// would delete static data and any future importer would fail
// The test is defined to pass if it doesn't segfault.
int TestVRMLImporter(int argc, char* argv[])
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
  // delete the importer and see if it can be run again
  importer->Delete();

  importer = svtkVRMLImporter::New();
  importer->SetRenderWindow(renWin);
  importer->SetFileName(fname);
  importer->Read();
  importer->Delete();

  delete[] fname;

  iren->Delete();
  renWin->Delete();
  ren1->Delete();

  return 0;
}
