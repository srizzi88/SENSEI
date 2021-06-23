/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContourTriangulatorCutter.cxx

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
#include "svtkContourTriangulator.h"
#include "svtkCutter.h"
#include "svtkDataSetMapper.h"
#include "svtkOutlineSource.h"
#include "svtkPlane.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTesting.h"

int TestContourTriangulatorCutter(int argc, char* argv[])
{
  svtkSmartPointer<svtkTesting> testHelper = svtkSmartPointer<svtkTesting>::New();
  testHelper->AddArguments(argc, argv);

  std::string tempDir = testHelper->GetTempDirectory();
  std::string tempBaseline = tempDir + "/TestContourTriangulatorCutter.png";

  double bounds[6] = { -210.0, +210.0, -210.0, +210.0, -100.0, +150.0 };
  svtkSmartPointer<svtkOutlineSource> outline = svtkSmartPointer<svtkOutlineSource>::New();
  outline->SetBounds(bounds);
  outline->GenerateFacesOn();

  svtkSmartPointer<svtkPlane> plane = svtkSmartPointer<svtkPlane>::New();
  plane->SetNormal(0.0, 0.0, -1.0);
  plane->SetOrigin(0.0, 0.0, 0.0);

  svtkSmartPointer<svtkCutter> cutter = svtkSmartPointer<svtkCutter>::New();
  cutter->SetInputConnection(outline->GetOutputPort());
  cutter->SetCutFunction(plane);

  svtkSmartPointer<svtkDataSetMapper> cutMapper = svtkSmartPointer<svtkDataSetMapper>::New();
  cutMapper->SetInputConnection(cutter->GetOutputPort());
  cutMapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> cutActor = svtkSmartPointer<svtkActor>::New();
  cutActor->SetMapper(cutMapper);
  cutActor->GetProperty()->SetColor(0, 0, 0);

  svtkSmartPointer<svtkContourTriangulator> poly = svtkSmartPointer<svtkContourTriangulator>::New();
  poly->TriangulationErrorDisplayOn();
  poly->SetInputConnection(cutter->GetOutputPort());

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
  renderer->AddActor(cutActor);

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
