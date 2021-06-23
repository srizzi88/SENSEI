/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContourTriangulator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how to use svtkContourTriangulator
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkContourFilter.h"
#include "svtkContourTriangulator.h"
#include "svtkDataSetMapper.h"
#include "svtkPNGReader.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTesting.h"

#include <string>

int TestContourTriangulator(int argc, char* argv[])
{
  svtkSmartPointer<svtkTesting> testHelper = svtkSmartPointer<svtkTesting>::New();
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();
  std::string tempDir = testHelper->GetTempDirectory();
  std::string inputFileName = dataRoot + "/Data/fullhead15.png";
  std::string tempBaseline = tempDir + "/TestContourTriangulator.png";

  svtkSmartPointer<svtkPNGReader> reader = svtkSmartPointer<svtkPNGReader>::New();
  if (!reader->CanReadFile(inputFileName.c_str()))
  {
    std::cerr << "Error: Could not read " << inputFileName << ".\n";
    return EXIT_FAILURE;
  }
  reader->SetFileName(inputFileName.c_str());
  reader->Update();

  svtkSmartPointer<svtkContourFilter> iso = svtkSmartPointer<svtkContourFilter>::New();
  iso->SetInputConnection(reader->GetOutputPort());
  iso->SetValue(0, 500);

  svtkSmartPointer<svtkDataSetMapper> isoMapper = svtkSmartPointer<svtkDataSetMapper>::New();
  isoMapper->SetInputConnection(iso->GetOutputPort());
  isoMapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> isoActor = svtkSmartPointer<svtkActor>::New();
  isoActor->SetMapper(isoMapper);
  isoActor->GetProperty()->SetColor(0, 0, 0);

  svtkSmartPointer<svtkContourTriangulator> poly = svtkSmartPointer<svtkContourTriangulator>::New();
  poly->SetInputConnection(iso->GetOutputPort());

  svtkSmartPointer<svtkDataSetMapper> polyMapper = svtkSmartPointer<svtkDataSetMapper>::New();
  polyMapper->SetInputConnection(poly->GetOutputPort());
  polyMapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> polyActor = svtkSmartPointer<svtkActor>::New();
  polyActor->SetMapper(polyMapper);
  polyActor->GetProperty()->SetColor(1.0, 1.0, 1.0);

  // Standard rendering classes
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  renderer->AddActor(polyActor);
  renderer->AddActor(isoActor);

  // Standard testing code.
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);

  svtkCamera* camera = renderer->GetActiveCamera();
  renderer->ResetCamera();
  camera->Azimuth(180);

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
