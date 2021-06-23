/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAVSucdReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkAVSucdReader.h>
#include <svtkDataSetMapper.h>
#include <svtkNew.h>
#include <svtkPointData.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkUnstructuredGrid.h>

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

int TestAVSucdReader(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "Required parameters: <filename>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string filename = argv[1];

  svtkNew<svtkAVSucdReader> reader;
  reader->SetFileName(filename.c_str());
  reader->Update();
  reader->Print(std::cout);
  reader->GetOutput()->Print(std::cout);

  svtkUnstructuredGrid* grid = svtkUnstructuredGrid::SafeDownCast(reader->GetOutput());
  grid->GetPointData()->SetActiveScalars("temperature");

  svtkNew<svtkDataSetMapper> mapper;
  mapper->SetInputData(reader->GetOutput());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarRange(grid->GetPointData()->GetScalars()->GetRange());

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  svtkNew<svtkRenderer> ren;
  ren->AddActor(actor);
  ren->SetBackground(0, 0, 0);

  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(ren);
  renWin->SetSize(300, 300);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();
  int r = svtkRegressionTestImage(renWin);
  if (r == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !r;
}
