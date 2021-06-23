/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTemporalXdmfReaderWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Description:
// This tests reading of a simple ADIOS2 bp file.

#include "svtkADIOS2CoreImageReader.h"

#include "svtkActor.h"
#include "svtkAlgorithm.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkCompositeRenderManager.h"
#include "svtkDataArray.h"
#include "svtkDataSetMapper.h"
#include "svtkImageData.h"
#include "svtkImageDataToPointSet.h"
#include "svtkMPIController.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkUnsignedIntArray.h"
#include "svtkXMLPMultiBlockDataWriter.h"

#include "svtkNew.h"
#include "svtkTestUtilities.h"

#include <sstream> // istringstream

struct TestArgs
{
  int* retval;
  int argc;
  char** argv;
};

void TestADIOS2BPReaderMPIMultiTimeSteps2D(svtkMultiProcessController* controller, void* _args)
{
  TestArgs* args = reinterpret_cast<TestArgs*>(_args);
  int argc = args->argc;
  char** argv = args->argv;
  *(args->retval) = 1;

  int currentRank = controller->GetLocalProcessId();
  svtkNew<svtkADIOS2CoreImageReader> reader;

  // Read the input data file
  char* filePath =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/ADIOS2/2D_HeatTransfer_10-step/sim.bp");

  if (!reader->CanReadFile(filePath))
  {
    std::cerr << "Cannot read file " << reader->GetFileName() << std::endl;
    return;
  }
  reader->SetFileName(filePath);
  delete[] filePath;

  reader->SetController(controller);

  reader->UpdateInformation();
  auto& availVars = reader->GetAvilableVariables();
  assert(availVars.size() == 1);
  // Get the dimension
  std::string varName = availVars.begin()->first;

  reader->SetDimensionArray("T");
  reader->SetTimeStepArray("T");

  reader->Update();

  svtkSmartPointer<svtkMultiBlockDataSet> output =
    svtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  assert(output->GetNumberOfBlocks() == 1);
  svtkSmartPointer<svtkMultiPieceDataSet> mpds =
    svtkMultiPieceDataSet::SafeDownCast(output->GetBlock(0));
  assert(mpds->GetNumberOfPieces() == 2);
  svtkSmartPointer<svtkImageData> image0 = svtkImageData::SafeDownCast(mpds->GetPiece(0));
  svtkSmartPointer<svtkImageData> image1 = svtkImageData::SafeDownCast(mpds->GetPiece(1));

  svtkNew<svtkImageDataToPointSet> imageToPointset;
  if (currentRank == 0)
  { // Rank0 should read one block as svtkImageData into index 0
    assert(image0);
    // assert(!image1);
    assert(image0->GetCellData()->GetNumberOfArrays() == 1);
    image0->GetCellData()->SetActiveScalars("T");
    imageToPointset->SetInputData(image0);
  }
  else if (currentRank == 1)
  { // Rank1 should read one block as svtkImageData into index 1
    assert(!image0);
    assert(image1);
    assert(image1->GetCellData()->GetNumberOfArrays() == 1);
    image1->GetCellData()->SetActiveScalars("T");
    imageToPointset->SetInputData(image1);
  }

  imageToPointset->Update();

  // Since I fail to find a proper mapper to render two svtkImageDatas inside
  // a svtkMultiPieceDataSet in a svtkMultiBlockDataSet, I render the image directly here
  svtkNew<svtkDataSetMapper> mapper;
  mapper->SetInputDataObject(imageToPointset->GetOutput());
  mapper->ScalarVisibilityOn();
  mapper->SetScalarRange(0, 2000);
  mapper->SetScalarModeToUseCellData();
  mapper->ColorByArrayComponent("T", 0);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();

  svtkNew<svtkCompositeRenderManager> prm;

  svtkSmartPointer<svtkRenderer> renderer;
  renderer.TakeReference(prm->MakeRenderer());
  renderer->AddActor(actor);
  renderer->SetBackground(0.5, 0.5, 0.5);
  renderer->GetActiveCamera()->Elevation(300);
  renderer->GetActiveCamera()->Yaw(60);
  renderer->ResetCamera();

  svtkSmartPointer<svtkRenderWindow> rendWin;
  rendWin.TakeReference(prm->MakeRenderWindow());
  rendWin->SetSize(600, 300);
  rendWin->SetPosition(0, 200 * controller->GetLocalProcessId());
  rendWin->AddRenderer(renderer);

  prm->SetRenderWindow(rendWin);
  prm->SetController(controller);
  prm->InitializePieces();
  prm->InitializeOffScreen(); // Mesa GL only

  if (controller->GetLocalProcessId() == 0)
  {
    rendWin->Render();

    // Do the test comparsion
    int retval = svtkRegressionTestImage(rendWin);
    if (retval == svtkRegressionTester::DO_INTERACTOR)
    {
      svtkNew<svtkRenderWindowInteractor> iren;
      iren->SetRenderWindow(rendWin);
      iren->Initialize();
      iren->Start();
      retval = svtkRegressionTester::PASSED;
    }
    std::cout << "retval after render=" << retval << std::endl;
    *(args->retval) = (retval == svtkRegressionTester::PASSED) ? 0 : 1;

    prm->StopServices();
  }
  else // not root node
  {
    prm->StartServices();
  }

  controller->Broadcast(args->retval, 1, 0);
}

int TestADIOS2BPReaderMPIMultiTimeSteps2D(int argc, char* argv[])
{
  int retval{ 1 };

  // Note that this will create a svtkMPIController if MPI
  // is configured, svtkThreadedController otherwise.
  svtkNew<svtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  svtkMultiProcessController::SetGlobalController(controller);

  TestArgs args;
  args.retval = &retval;
  args.argc = argc;
  args.argv = argv;

  controller->SetSingleMethod(TestADIOS2BPReaderMPIMultiTimeSteps2D, &args);
  controller->SingleMethodExecute();

  controller->Finalize();

  return retval;
}
