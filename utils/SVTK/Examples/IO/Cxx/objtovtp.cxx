/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSingleVTPExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkNew.h"
#include "svtkOBJImporter.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSingleVTPExporter.h"
#include "svtksys/SystemTools.hxx"

#include <sstream>

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "expected objtosvtk File1.obj [File2.obj.mtl]" << std::endl;
    return -1;
  }

  std::string filenameOBJ(argv[1]);

  std::string filenameMTL;

  if (argc >= 3)
  {
    filenameMTL = argv[2];
  }
  std::string texturePath = svtksys::SystemTools::GetFilenamePath(filenameOBJ);

  svtkNew<svtkOBJImporter> importer;

  importer->SetFileName(filenameOBJ.data());
  importer->SetFileNameMTL(filenameMTL.data());
  importer->SetTexturePath(texturePath.data());

  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);
  importer->SetRenderWindow(renWin);
  importer->Update();

  renWin->SetSize(800, 600);
  ren->SetBackground(0.4, 0.5, 0.6);
  ren->ResetCamera();
  renWin->Render();

  // export to a single svtk file
  svtkNew<svtkSingleVTPExporter> exporter;

  std::string outputPrefix = "o2v";
  outputPrefix += svtksys::SystemTools::GetFilenameWithoutLastExtension(filenameOBJ);

  exporter->SetFilePrefix(outputPrefix.c_str());
  exporter->SetRenderWindow(renWin);
  exporter->Write();

  iren->Start();

  return 0;
}
