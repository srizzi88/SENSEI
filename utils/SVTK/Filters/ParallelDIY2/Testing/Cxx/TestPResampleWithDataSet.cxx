/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResampleWithDataset.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPResampleWithDataSet.h"

#include "svtkActor.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositePolyDataMapper.h"
#include "svtkCompositeRenderManager.h"
#include "svtkCylinder.h"
#include "svtkExtentTranslator.h"
#include "svtkMPIController.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphere.h"
#include "svtkTableBasedClipDataSet.h"
#include "svtkTransform.h"
#include "svtkTransformFilter.h"

namespace
{

void CreateInputDataSet(
  svtkMultiBlockDataSet* dataset, int rank, int numberOfProcs, int blocksPerProc)
{
  int numPieces = blocksPerProc * numberOfProcs;
  dataset->SetNumberOfBlocks(numPieces);

  svtkNew<svtkExtentTranslator> extentTranslator;
  extentTranslator->SetWholeExtent(-16, 16, -16, 16, -16, 16);
  extentTranslator->SetNumberOfPieces(numPieces);
  extentTranslator->SetSplitModeToBlock();

  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-16, 16, -16, 16, -16, 16);
  wavelet->SetCenter(0, 0, 0);

  svtkNew<svtkCylinder> cylinder;
  cylinder->SetCenter(0, 0, 0);
  cylinder->SetRadius(15);
  cylinder->SetAxis(0, 1, 0);
  svtkNew<svtkTableBasedClipDataSet> clipCyl;
  clipCyl->SetClipFunction(cylinder);
  clipCyl->InsideOutOn();

  svtkNew<svtkSphere> sphere;
  sphere->SetCenter(0, 0, 4);
  sphere->SetRadius(12);
  svtkNew<svtkTableBasedClipDataSet> clipSphr;
  clipSphr->SetInputConnection(clipCyl->GetOutputPort());
  clipSphr->SetClipFunction(sphere);

  svtkNew<svtkTransform> transform;
  transform->RotateZ(45);
  svtkNew<svtkTransformFilter> transFilter;
  transFilter->SetInputConnection(clipSphr->GetOutputPort());
  transFilter->SetTransform(transform);

  for (int i = 0; i < blocksPerProc; ++i)
  {
    int piece = (rank * blocksPerProc) + i;

    int blockExtent[6];
    extentTranslator->SetPiece(piece);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(blockExtent);

    wavelet->UpdateExtent(blockExtent);
    clipCyl->SetInputData(wavelet->GetOutputDataObject(0));
    transFilter->Update();

    svtkDataObject* block = transFilter->GetOutputDataObject(0)->NewInstance();
    block->DeepCopy(transFilter->GetOutputDataObject(0));
    dataset->SetBlock(piece, block);
    block->Delete();
  }
}

void CreateSourceDataSet(
  svtkMultiBlockDataSet* dataset, int rank, int numberOfProcs, int blocksPerProc)
{
  int numPieces = blocksPerProc * numberOfProcs;
  dataset->SetNumberOfBlocks(numPieces);

  svtkNew<svtkExtentTranslator> extentTranslator;
  extentTranslator->SetWholeExtent(-22, 22, -22, 22, -16, 16);
  extentTranslator->SetNumberOfPieces(numPieces);
  extentTranslator->SetSplitModeToBlock();

  svtkNew<svtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-22, 22, -22, 22, -16, 16);
  wavelet->SetCenter(0, 0, 0);

  for (int i = 0; i < blocksPerProc; ++i)
  {
    int piece = (rank * blocksPerProc) + i;

    int blockExtent[6];
    extentTranslator->SetPiece(piece);
    extentTranslator->PieceToExtent();
    extentTranslator->GetExtent(blockExtent);

    wavelet->UpdateExtent(blockExtent);

    svtkDataObject* block = wavelet->GetOutputDataObject(0)->NewInstance();
    block->DeepCopy(wavelet->GetOutputDataObject(0));
    dataset->SetBlock(piece, block);
    block->Delete();
  }
}

} // anonymous namespace

int TestPResampleWithDataSet(int argc, char* argv[])
{
  svtkNew<svtkMPIController> controller;
  controller->Initialize(&argc, &argv);

  int numProcs = controller->GetNumberOfProcesses();
  int rank = controller->GetLocalProcessId();

  // create input dataset
  svtkNew<svtkMultiBlockDataSet> input;
  CreateInputDataSet(input, rank, numProcs, 3);

  svtkNew<svtkMultiBlockDataSet> source;
  CreateSourceDataSet(source, rank, numProcs, 5);

  svtkNew<svtkPResampleWithDataSet> resample;
  resample->SetController(controller);
  resample->SetInputData(input);
  resample->SetSourceData(source);
  resample->Update();

  // Render
  svtkNew<svtkCompositeDataGeometryFilter> toPoly;
  toPoly->SetInputConnection(resample->GetOutputPort());

  double range[2];
  toPoly->Update();
  toPoly->GetOutput()->GetPointData()->GetArray("RTData")->GetRange(range);

  svtkNew<svtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(toPoly->GetOutputPort());
  mapper->SetScalarRange(range);

  // Setup parallel rendering
  svtkNew<svtkCompositeRenderManager> prm;
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::Take(prm->MakeRenderer());
  svtkSmartPointer<svtkRenderWindow> renWin =
    svtkSmartPointer<svtkRenderWindow>::Take(prm->MakeRenderWindow());
  renWin->AddRenderer(renderer);
  renWin->DoubleBufferOn();
  renWin->SetMultiSamples(0);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  prm->SetRenderWindow(renWin);
  prm->SetController(controller);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  int r1 = svtkTesting::PASSED;
  if (rank == 0)
  {
    prm->ResetAllCameras();

    std::cout << "Test with RegularPartition" << std::endl;
    renWin->Render();
    r1 = svtkRegressionTester::Test(argc, argv, renWin, 10);
    if (!r1)
    {
      std::cout << "Test with RegularPartition failed" << std::endl;
    }
    else if (r1 == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
    prm->StopServices();
  }
  else
  {
    prm->StartServices();
  }
  controller->Barrier();

  resample->UseBalancedPartitionForPointsLookupOn();
  int r2 = svtkTesting::PASSED;
  if (rank == 0)
  {
    prm->ResetAllCameras();

    std::cout << "Test with BalancedPartition" << std::endl;
    renWin->Render();
    r2 = svtkRegressionTester::Test(argc, argv, renWin, 10);
    if (!r2)
    {
      std::cout << "Test with BalancedPartition failed" << std::endl;
    }
    else if (r2 == svtkRegressionTester::DO_INTERACTOR)
    {
      iren->Start();
    }
    prm->StopServices();
  }
  else
  {
    prm->StartServices();
  }
  controller->Barrier();

  int status = r1 && r2;
  controller->Broadcast(&status, 1, 0);
  controller->Finalize();

  return !status;
}
