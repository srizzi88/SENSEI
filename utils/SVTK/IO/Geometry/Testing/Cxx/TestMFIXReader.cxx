/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMFIXReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <svtkActor.h>
#include <svtkCellData.h>
#include <svtkDataSetMapper.h>
#include <svtkExecutive.h>
#include <svtkMFIXReader.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkUnstructuredGrid.h>

#include <svtkRegressionTestImage.h>
#include <svtkTestErrorObserver.h>
#include <svtkTestUtilities.h>

int TestMFIXReader(int argc, char* argv[])
{
  // Read file name.
  char* filename = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/MFIXReader/BUB01.RES");

  svtkSmartPointer<svtkTest::ErrorObserver> errorObserver1 =
    svtkSmartPointer<svtkTest::ErrorObserver>::New();
  svtkSmartPointer<svtkTest::ErrorObserver> errorObserver2 =
    svtkSmartPointer<svtkTest::ErrorObserver>::New();

  svtkSmartPointer<svtkMFIXReader> reader = svtkSmartPointer<svtkMFIXReader>::New();
  reader->AddObserver(svtkCommand::ErrorEvent, errorObserver1);
  reader->GetExecutive()->AddObserver(svtkCommand::ErrorEvent, errorObserver2);

  // Update without a filename should cause an error
  reader->Update();
  errorObserver1->CheckErrorMessage("No filename specified");

  reader->SetFileName(filename);
  delete[] filename;

  reader->Update();

  std::cout << "Testing reader with file: " << reader->GetFileName() << std::endl;
  std::cout << "There are " << reader->GetNumberOfPoints() << " number of points" << std::endl;
  std::cout << "There are " << reader->GetNumberOfCells() << " number of cells" << std::endl;
  std::cout << "There are " << reader->GetNumberOfCellFields() << " number of cell fields"
            << std::endl;
  reader->SetTimeStep(reader->GetNumberOfTimeSteps() / 2);
  std::cout << "The timestep is  " << reader->GetTimeStep() << std::endl;
  reader->SetTimeStepRange(0, reader->GetNumberOfTimeSteps() - 1);
  std::cout << "The time step range is: " << reader->GetTimeStepRange()[0] << " to "
            << reader->GetTimeStepRange()[1] << std::endl;
  // Exercise Cell Arrays

  // 1) Default array settings
  int numberOfCellArrays = reader->GetNumberOfCellArrays();
  std::cout << "----- Default array settings" << std::endl;
  for (int i = 0; i < numberOfCellArrays; ++i)
  {
    const char* name = reader->GetCellArrayName(i);
    std::cout << "  Cell Array: " << i << " is named " << name << " and is "
              << (reader->GetCellArrayStatus(name) ? "Enabled" : "Disabled") << std::endl;
  }

  // 2) Disable one array
  std::cout << "----- Disable one array" << std::endl;
  const char* arrayName = reader->GetCellArrayName(0);
  reader->SetCellArrayStatus(arrayName, 0);
  if (reader->GetCellArrayStatus(arrayName) != 0)
  {
    std::cout << "ERROR:  Cell Array: "
              << "0"
              << " is named " << arrayName << " and should be disabled" << std::endl;
    return EXIT_FAILURE;
  }

  // 3) Disable all arrays
  std::cout << "----- Disable all arrays" << std::endl;
  reader->DisableAllCellArrays();
  for (int i = 0; i < numberOfCellArrays; ++i)
  {
    const char* name = reader->GetCellArrayName(i);
    if (reader->GetCellArrayStatus(name) != 0)
    {
      std::cout << "ERROR: "
                << "  Cell Array: " << i << " is named " << name << " and should be disabled"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  // 4) Enable one array
  std::cout << "----- Enable one array" << std::endl;
  arrayName = reader->GetCellArrayName(0);
  reader->SetCellArrayStatus(arrayName, 1);
  if (reader->GetCellArrayStatus(arrayName) != 1)
  {
    std::cout << "ERROR:  Cell Array: "
              << "0"
              << " is named " << arrayName << " and should be disabled" << std::endl;
    return EXIT_FAILURE;
  }

  // 5) Enable all arrays
  std::cout << "----- Enable all arrays" << std::endl;
  reader->EnableAllCellArrays();
  for (int i = 0; i < numberOfCellArrays; ++i)
  {
    const char* name = reader->GetCellArrayName(i);
    if (reader->GetCellArrayStatus(name) != 1)
    {
      std::cout << "ERROR: "
                << "  Cell Array: " << i << " is named " << name << " and should be enabled"
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  reader->Print(std::cout);

  // Visualize
  svtkSmartPointer<svtkDataSetMapper> mapper = svtkSmartPointer<svtkDataSetMapper>::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  mapper->SetScalarRange(reader->GetOutput()->GetScalarRange());
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
