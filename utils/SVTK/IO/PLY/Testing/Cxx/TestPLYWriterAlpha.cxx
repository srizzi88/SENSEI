/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPLYWriterAlpha.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkPLYWriter with alpha.

#include "svtkElevationFilter.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkPLYReader.h"
#include "svtkPLYWriter.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"

int TestPLYWriterAlpha(int argc, char* argv[])
{
  // Test temporary directory
  char* tempDir =
    svtkTestUtilities::GetArgOrEnvOrDefault("-T", argc, argv, "SVTK_TEMP_DIR", "Testing/Temporary");
  if (!tempDir)
  {
    std::cout << "Could not determine temporary directory.\n";
    return EXIT_FAILURE;
  }

  std::string testDirectory = tempDir;
  delete[] tempDir;

  const std::string outputfile = testDirectory + std::string("/") + std::string("plyAlpha.ply");

  svtkNew<svtkSphereSource> sphere;
  sphere->SetPhiResolution(20);
  sphere->SetThetaResolution(20);

  svtkNew<svtkElevationFilter> elevation;
  elevation->SetInputConnection(sphere->GetOutputPort());
  elevation->SetLowPoint(-0.5, -0.5, -0.5);
  elevation->SetHighPoint(0.5, 0.5, 0.5);

  svtkNew<svtkLookupTable> lut;
  lut->SetTableRange(0, 1);
  lut->SetAlphaRange(0, 1.0);
  lut->Build();

  svtkNew<svtkPLYWriter> writer;
  writer->SetFileName(outputfile.c_str());
  writer->SetFileTypeToBinary();
  writer->EnableAlphaOn();
  writer->SetColorModeToDefault();
  writer->SetArrayName("Elevation");
  writer->SetLookupTable(lut);
  writer->SetInputConnection(elevation->GetOutputPort());
  writer->Write();

  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(outputfile.c_str());

  // Create a mapper.
  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->ScalarVisibilityOn();

  // Create the actor.
  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  // Basic visualisation.
  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderer> ren;
  renWin->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
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
