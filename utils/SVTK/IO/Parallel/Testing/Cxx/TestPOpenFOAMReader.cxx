/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSimplePointsReaderWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <svtkCellData.h>
#include <svtkDataSetMapper.h>
#include <svtkMultiBlockDataSet.h>
#include <svtkPOpenFOAMReader.h>
#include <svtkPointData.h>
#include <svtkProperty.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkUnstructuredGrid.h>

#include <svtkRegressionTestImage.h>
#include <svtkTestUtilities.h>

int TestPOpenFOAMReader(int argc, char* argv[])
{
  // Read file name.
  char* filename =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/OpenFOAM/cavity/cavity.foam");

  // Read the file
  svtkSmartPointer<svtkPOpenFOAMReader> reader = svtkSmartPointer<svtkPOpenFOAMReader>::New();
  reader->SetFileName(filename);
  delete[] filename;
  reader->Update();
  reader->SetTimeValue(.5);
  //  reader->CreateCellToPointOn();
  reader->ReadZonesOn();
  reader->Update();
  reader->Print(std::cout);
  reader->GetOutput()->Print(std::cout);
  reader->GetOutput()->GetBlock(0)->Print(std::cout);

  // 1) Default array settings
  int numberOfCellArrays = reader->GetNumberOfCellArrays();
  std::cout << "----- Default array settings" << std::endl;
  for (int i = 0; i < numberOfCellArrays; ++i)
  {
    const char* name = reader->GetCellArrayName(i);
    std::cout << "  Cell Array: " << i << " is named " << name << " and is "
              << (reader->GetCellArrayStatus(name) ? "Enabled" : "Disabled") << std::endl;
  }

  int numberOfPointArrays = reader->GetNumberOfPointArrays();
  std::cout << "----- Default array settings" << std::endl;
  for (int i = 0; i < numberOfPointArrays; ++i)
  {
    const char* name = reader->GetPointArrayName(i);
    std::cout << "  Point Array: " << i << " is named " << name << " and is "
              << (reader->GetPointArrayStatus(name) ? "Enabled" : "Disabled") << std::endl;
  }

  int numberOfLagrangianArrays = reader->GetNumberOfLagrangianArrays();
  std::cout << "----- Default array settings" << std::endl;
  for (int i = 0; i < numberOfLagrangianArrays; ++i)
  {
    const char* name = reader->GetLagrangianArrayName(i);
    std::cout << "  Lagrangian Array: " << i << " is named " << name << " and is "
              << (reader->GetLagrangianArrayStatus(name) ? "Enabled" : "Disabled") << std::endl;
  }

  int numberOfPatchArrays = reader->GetNumberOfPatchArrays();
  std::cout << "----- Default array settings" << std::endl;
  for (int i = 0; i < numberOfPatchArrays; ++i)
  {
    const char* name = reader->GetPatchArrayName(i);
    std::cout << "  Patch Array: " << i << " is named " << name << " and is "
              << (reader->GetPatchArrayStatus(name) ? "Enabled" : "Disabled") << std::endl;
  }

  svtkUnstructuredGrid* block0 = svtkUnstructuredGrid::SafeDownCast(reader->GetOutput()->GetBlock(0));
  block0->GetCellData()->SetActiveScalars("p");
  std::cout << "Scalar range: " << block0->GetCellData()->GetScalars()->GetRange()[0] << ", "
            << block0->GetCellData()->GetScalars()->GetRange()[1] << std::endl;

  // Visualize
  svtkSmartPointer<svtkDataSetMapper> mapper = svtkSmartPointer<svtkDataSetMapper>::New();
  mapper->SetInputData(block0);
  mapper->SetScalarRange(block0->GetScalarRange());

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renderWindow = svtkSmartPointer<svtkRenderWindow>::New();
  renderWindow->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> renderWindowInteractor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow);

  renderer->AddActor(actor);
  renderer->SetBackground(.2, .4, .6);

  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
  }

  return !retVal;
}
