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
#include "svtkElevationFilter.h"
#include "svtkGLTFExporter.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include <svtksys/SystemTools.hxx>

#include <cstdlib>

namespace
{
size_t fileSize(const std::string& filename)
{
  size_t size = 0;
  FILE* f = svtksys::SystemTools::Fopen(filename, "r");
  if (f)
  {
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fclose(f);
  }
  else
  {
    std::cerr << "Error: cannot open file " << filename << std::endl;
  }

  return size;
}
}

int TestGLTFExporter(int argc, char* argv[])
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

  std::string filename = testDirectory + std::string("/") + std::string("Export");

  svtkNew<svtkSphereSource> sphere;
  svtkNew<svtkElevationFilter> elev;
  elev->SetInputConnection(sphere->GetOutputPort());
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(elev->GetOutputPort());
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  svtkNew<svtkRenderWindow> window;
  window->AddRenderer(renderer);
  window->Render();

  filename += ".gltf";

  svtkNew<svtkGLTFExporter> exporter;
  exporter->SetRenderWindow(window);
  exporter->SetFileName(filename.c_str());
  exporter->InlineDataOn();
  // for debugging uncomment below
  // std::string estring = exporter->WriteToString();
  // cout << estring;
  exporter->Write();

  size_t correctSize = fileSize(filename);
  if (correctSize == 0)
  {
    return EXIT_FAILURE;
  }

  actor->VisibilityOff();
  exporter->Write();
  size_t noDataSize = fileSize(filename);
  if (noDataSize == 0)
  {
    return EXIT_FAILURE;
  }

  if (noDataSize >= correctSize)
  {
    std::cerr << "Error: file should contain data for a visible actor"
                 "and not for a hidden one."
              << std::endl;
    return EXIT_FAILURE;
  }

  actor->VisibilityOn();
  actor->SetMapper(nullptr);
  exporter->Write();
  size_t size = fileSize(filename);
  if (size == 0)
  {
    return EXIT_FAILURE;
  }
  if (size > noDataSize)
  {
    std::cerr << "Error: file should not contain geometry"
                 " (actor has no mapper)"
              << std::endl;
    return EXIT_FAILURE;
  }

  actor->SetMapper(mapper);
  mapper->RemoveAllInputConnections(0);
  exporter->Write();
  size = fileSize(filename);
  if (size == 0)
  {
    return EXIT_FAILURE;
  }
  if (size > noDataSize)
  {
    std::cerr << "Error: file should not contain geometry"
                 " (mapper has no input)"
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
