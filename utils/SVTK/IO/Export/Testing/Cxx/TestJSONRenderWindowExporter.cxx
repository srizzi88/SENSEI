/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkLight.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkWindowNode.h"
#include "svtksys/SystemTools.hxx"

#include "svtkArchiver.h"
#include "svtkJSONRenderWindowExporter.h"

int TestJSONRenderWindowExporter(int argc, char* argv[])
{
  char* tempDir =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }
  std::string testDirectory = tempDir;
  delete[] tempDir;

  std::string filename = testDirectory + std::string("/") + std::string("ExportVtkJS");

  svtkNew<svtkSphereSource> sphere;
  svtkNew<svtkPolyDataMapper> pmap;
  pmap->SetInputConnection(sphere->GetOutputPort());

  svtkNew<svtkRenderWindow> rwin;

  svtkNew<svtkRenderer> ren;
  rwin->AddRenderer(ren);

  svtkNew<svtkLight> light;
  ren->AddLight(light);

  svtkNew<svtkActor> actor;
  ren->AddActor(actor);

  actor->SetMapper(pmap);

  svtkNew<svtkJSONRenderWindowExporter> exporter;
  exporter->GetArchiver()->SetArchiveName(filename.c_str());
  exporter->SetRenderWindow(rwin);
  exporter->Write();

  svtksys::SystemTools::RemoveADirectory(filename.c_str());

  return EXIT_SUCCESS;
}
